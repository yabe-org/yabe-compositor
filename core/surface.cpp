#include "surface.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>
#include <optional>
#include <utility>
#include <vector>

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/server.hpp"

namespace yabe::core {
Surface::Surface(wl_resource* resource) : resource_(resource) {
  LOG_DBG("wl_surface#{}: created", wl_resource_get_id(resource));
}
Surface::~Surface() {
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
    const std::lock_guard lock(this->buffer_mutex_);
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
    }

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
  bool                        should_upload = false;
  {
    const std::lock_guard lock(this->buffer_mutex_);
    if (!this->current_.buffer) {
      return;
    }
    current_buffer = *this->current_.buffer;
    const auto current_buffer_resource_lock =
        this->current_.buffer.value()->lock_resource();
    frame_callbacks.swap(this->current_.frame_callbacks);
    release_callback.swap(this->current_.release_callback);
    if (this->current_.buffer.value()->resource() &&
        !this->is_texture_uploaded_) {
      should_upload              = true;
      this->is_texture_uploaded_ = true;
    }
  }

  if (should_upload) {
    auto* shm_buffer      = current_buffer->shm_buffer();
    this->texture_width_  = wl_shm_buffer_get_width(shm_buffer);
    this->texture_height_ = wl_shm_buffer_get_height(shm_buffer);
    auto* data            = wl_shm_buffer_get_data(shm_buffer);
    auto  format          = wl_shm_buffer_get_format(shm_buffer);
    auto  stride          = wl_shm_buffer_get_stride(shm_buffer);
    wl_shm_buffer_begin_access(shm_buffer);
    this->texture_.upload(
        data, format, this->texture_width_, this->texture_height_, stride);
    wl_shm_buffer_end_access(shm_buffer);
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

void Surface::map(std::optional<std::shared_ptr<scene::SceneNode>> parent,
    scene::SurfaceNode::Anchor                                     anchor) {
  if (this->surface_node_.has_value()) {
    return;
  }
  this->surface_node_ =
      std::make_shared<scene::SurfaceNode>(this->shared_from_this(), anchor);
  if (this->current_.buffer.has_value()) {
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
}  // namespace yabe::core
