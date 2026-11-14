#include "surface.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/scene/scene.hpp"
#include "core/server.hpp"

namespace yabe::core {
Surface::Surface(wl_resource* resource) : resource_(resource) {
  LOG_DBG("wl_surface#{}: created", wl_resource_get_id(resource));
}
Surface::~Surface() {
  this->events_.destroyed.emit(nullptr);
  this->unmap();
  LOG_DBG("wl_surface#{}: destroyed", wl_resource_get_id(this->resource_));
}

void Surface::set_frame_callback(wl_resource* resource) {
  this->pending_.frame_callbacks.emplace_back(resource);
}
void Surface::set_input_region(std::optional<Region> region) {
  this->has_pending_input_region_ = true;
  this->pending_.input_region     = std::move(region);
}
void Surface::set_release_callback(wl_resource* resource) {
  this->pending_.release_callback = resource;
}
void Surface::attach(wl_resource* resource) {
  this->pending_.buffer = std::make_shared<Buffer>(resource);
}
void Surface::damage(int32_t x, int32_t y, int32_t width, int32_t height) {
  this->pending_.damage.add(x, y, width, height);
}

void Surface::commit() {
  const auto is_valid_buffer =
      [](const std::optional<std::shared_ptr<Buffer>>& buffer) {
        if (!buffer.has_value()) {
          return false;
        }
        if (buffer.value()->resource() == nullptr) {
          return false;
        }
        return true;
      };
  std::optional<std::shared_ptr<Buffer>> buffer_to_release;
  {
    const auto lock = this->mutex_.lock();
    if (this->pending_.buffer.has_value()) {
      // if the current buffer exists, is not uploaded yet, and is different
      // from newly attached buffer, current buffer will no longer be used
      if (is_valid_buffer(this->current_.buffer) &&
          !this->is_texture_uploaded_ &&
          this->current_.buffer.value()->resource() !=
              this->pending_.buffer.value()->resource()) {
        buffer_to_release = this->current_.buffer;
      }
      // previous `current.buffer` will be released (replaced),
      // so we can send done current release_callback
      if (this->current_.release_callback) {
        wl_callback_send_done(*this->current_.release_callback, 0);
        wl_resource_destroy(*this->current_.release_callback);
        this->current_.release_callback.reset();
      }
      this->is_texture_uploaded_ = false;
      this->current_.buffer      = this->pending_.buffer;
      this->pending_.buffer      = std::nullopt;

      if (is_valid_buffer(this->current_.buffer)) {
        this->current_.damage.add(this->pending_.damage);
      } else {
        // the contents are removed, so the next buffer must be uploaded
        // entirely regardless of its damage
        this->current_.damage.add(0, 0, std::numeric_limits<int32_t>::max(),
            std::numeric_limits<int32_t>::max());
      }
    }
    this->pending_.damage.clear();

    this->current_.frame_callbacks.insert(this->current_.frame_callbacks.end(),
        std::move_iterator(this->pending_.frame_callbacks.begin()),
        std::move_iterator(this->pending_.frame_callbacks.end()));
    this->pending_.frame_callbacks.clear();

    if (this->pending_.release_callback.has_value()) {
      this->current_.release_callback = this->pending_.release_callback;
      this->pending_.release_callback = std::nullopt;
    }

    if (this->has_pending_input_region_) {
      this->has_pending_input_region_ = false;
      this->current_.input_region     = this->pending_.input_region;
      this->pending_.input_region     = std::nullopt;
    }
  }

  if (buffer_to_release.has_value()) {
    wl_buffer_send_release(buffer_to_release.value()->resource());
  }

  if (const auto role = this->role_.lock()) {
    role->commit();
  }
}

void Surface::upload_texture_if_needed() {
  std::shared_ptr<Buffer>     current_buffer;
  std::vector<wl_resource*>   frame_callbacks;
  std::optional<wl_resource*> release_callback;
  Region                      damage;
  bool                        should_upload = false;
  {
    const auto lock = this->mutex_.lock();
    if (!this->current_.buffer) {
      return;
    }
    current_buffer = *this->current_.buffer;
    frame_callbacks.swap(this->current_.frame_callbacks);
    release_callback.swap(this->current_.release_callback);
    if (!this->is_texture_uploaded_) {
      const auto current_buffer_resource_lock = current_buffer->lock_resource();
      if (current_buffer->resource()) {
        should_upload              = true;
        this->is_texture_uploaded_ = true;
        std::swap(damage, this->current_.damage);
      }
    }
  }

  if (should_upload && !this->upload_texture(*current_buffer, damage)) {
    // the buffer has been destroyed before uploading,
    // so the damage must be uploaded together with the next buffer
    const auto lock = this->mutex_.lock();
    damage.add(this->current_.damage);
    std::swap(damage, this->current_.damage);
  }

  if (this->surface_node_.has_value()) {
    auto& scale = this->surface_node_.value()->geometry().scale();
    scale.x     = *this->texture_width_.to_world_unit();
    scale.y     = *this->texture_height_.to_world_unit();
  }

  const auto weak_surface = this->weak_from_this();
  server::get().enqueue_wayland_thread_task(
      [weak_surface, should_upload, buffer = std::move(current_buffer),
          frame_callbacks = std::move(frame_callbacks), release_callback]() {
        const auto surface = weak_surface.lock();
        if (!surface) {
          return;
        }
        if (should_upload && buffer->resource()) {
          wl_buffer_send_release(buffer->resource());
        }
        for (auto* frame_callback : frame_callbacks) {
          wl_callback_send_done(frame_callback, common::now_msec());
          wl_resource_destroy(frame_callback);
        }
        if (release_callback.has_value()) {
          wl_callback_send_done(release_callback.value(), 0);
          wl_resource_destroy(release_callback.value());
        }
      });
}

bool Surface::upload_texture(Buffer& buffer, Region& damage) {
  // hold the lock while uploading to prevent the buffer from being destroyed
  const auto buffer_resource_lock = buffer.lock_resource();
  auto*      shm_buffer           = buffer.shm_buffer();
  if (!buffer.resource() || !shm_buffer) {
    return false;
  }
  this->texture_width_  = wl_shm_buffer_get_width(shm_buffer);
  this->texture_height_ = wl_shm_buffer_get_height(shm_buffer);
  auto* data            = wl_shm_buffer_get_data(shm_buffer);
  auto  format          = wl_shm_buffer_get_format(shm_buffer);
  auto  stride          = wl_shm_buffer_get_stride(shm_buffer);
  damage.intersect(0, 0, *this->texture_width_, *this->texture_height_);
  wl_shm_buffer_begin_access(shm_buffer);
  this->texture_.upload_damaged(data, format, this->texture_width_,
      this->texture_height_, stride, damage.boxes());
  wl_shm_buffer_end_access(shm_buffer);
  return true;
}

void Surface::map(std::optional<std::shared_ptr<scene::SceneNode>> parent,
    scene::SurfaceNode::Anchor                                     anchor) {
  if (this->surface_node_.has_value()) {
    return;
  }
  this->surface_node_ =
      std::make_shared<scene::SurfaceNode>(this->shared_from_this(), anchor);
  if (this->has_buffer()) {
    auto& scale = this->surface_node_.value()->geometry().scale();
    scale.x     = *this->texture_width_.to_world_unit();
    scale.y     = *this->texture_height_.to_world_unit();
  }
  if (parent.has_value()) {
    const auto lock = scene::Scene::mutex.lock();
    parent.value()->add_child(this->surface_node_.value());
  } else {
    server::get().workspace.current_scene().insert(this->surface_node_.value());
  }
  const auto output = server::get().output_map.find(this->resource_->client);
  if (output != server::get().output_map.end()) {
    wl_surface_send_enter(this->resource_, output->second);
  }
}
void Surface::unmap() {
  if (!this->surface_node_.has_value()) {
    return;
  }
  server::get().workspace.current_scene().erase(this->surface_node_.value());
  this->surface_node_ = std::nullopt;
  const auto output   = server::get().output_map.find(this->resource_->client);
  if (output != server::get().output_map.end()) {
    wl_surface_send_leave(this->resource_, output->second);
  }
}
SurfaceSetParentResult Surface::set_parent(
    std::optional<std::shared_ptr<Surface>> new_parent) {
  // FIXME: this function does not set the parent
  // if this or the parent is not yet mapped
  if (!this->surface_node_) {
    return SurfaceSetParentResult::FAILED_OTHER;
  }

  if (!new_parent) {
    {
      const auto lock = core::scene::Scene::mutex.lock();
      // chwck parent existence with SurfaceNode::parent(), not Surface::parent_
      // (Surface::parent_ have expired when ~Surface() is called,
      // but Surface::surface_node_ still alives)
      if (this->surface_node_.value()->parent().expired()) {
        return SurfaceSetParentResult::FAILED_OTHER;
      }
    }
    this->surface_node_.value()->detach_from_parent();
    this->parent_.reset();
    return SurfaceSetParentResult::SUCCEEDED;
  }

  if (*new_parent == this->parent_.lock()) {
    return SurfaceSetParentResult::FAILED_OTHER;
  }
  if (this->is_ancestor_of(*new_parent)) {
    return SurfaceSetParentResult::
        FAILED_SELF_IS_SAME_AS_OR_ANCESTOR_OF_NEW_PARENT;
  }
  const auto new_parent_node = new_parent.value()->surface_node_;
  if (!new_parent_node.has_value()) {
    this->surface_node_.value()->detach_from_parent();
    this->parent_.reset();
    return SurfaceSetParentResult::SUCCEEDED;
  }
  this->parent_                    = *new_parent;
  core::scene::Scene* scene        = nullptr;
  core::scene::Scene* parent_scene = nullptr;
  {
    const auto lock = core::scene::Scene::mutex.lock();
    scene           = this->surface_node_.value()->owner();
    parent_scene    = new_parent_node.value()->owner();
  }
  scene->erase(this->surface_node_.value());

  {
    const auto lock = core::scene::Scene::mutex.lock();
    // reset pos&rot so that parent can decide them of the child
    this->surface_node_.value()->geometry().pos()      = glm::vec3(0.F);
    this->surface_node_.value()->geometry().rotation() = glm::quat();
    new_parent_node.value()->add_child(this->surface_node_.value());
  }
  // if parent is keyboard-focused, newly added child should be focused now
  const auto keyboard_focused_surface =
      core::server::get().seat.keyboard.get_focused_surface().lock();
  if (keyboard_focused_surface == *new_parent) {
    parent_scene->set_active_node(this->surface_node_.value());
  }
  return SurfaceSetParentResult::SUCCEEDED;
}

void Surface::add_listener_destroyed(
    common::Listener<std::nullptr_t*>& listener) {
  this->events_.destroyed.add_listener(listener);
}

bool Surface::is_ancestor_of(const std::shared_ptr<Surface>& other) const {
  for (auto ancestor = other; ancestor; ancestor = ancestor->parent_.lock()) {
    if (ancestor.get() == this) {
      return true;
    }
  }
  return false;
}

}  // namespace yabe::core
