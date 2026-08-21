#include "surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>
#include <optional>
#include <utility>
#include <vector>

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/server.hpp"
#include "wayland-server-core.h"

namespace yabe::core {
Surface::Surface(wl_resource* resource) : resource_(resource) {
  LOG_DBG("wl_surface#{}: created", wl_resource_get_id(resource));
}
Surface::~Surface() {
  this->unmap();
  LOG_DBG("wl_surface#{}: destroyed", wl_resource_get_id(this->resource_));
}

void Surface::set_frame_callback(wl_resource* resource) {
  const std::lock_guard lock(this->buffer_mutex_);
  this->pending_.frame_callbacks.emplace_back(resource);
}
void Surface::set_input_region(std::optional<Region> region) {
  this->has_pending_input_region_ = true;
  this->pending_.input_region     = std::move(region);
}
void Surface::set_release_callback(wl_resource* resource) {
  const std::lock_guard lock(this->buffer_mutex_);
  this->pending_.release_callback = resource;
}

void Surface::attach(wl_resource* resource) {
  const std::lock_guard lock(this->buffer_mutex_);
  this->pending_.buffer = std::make_optional<Buffer>(resource);
}

void Surface::commit() {
  const auto is_valid_buffer = [](const std::optional<Buffer>& buffer) {
    if (!buffer.has_value()) {
      return false;
    }
    if (buffer->resource == nullptr) {
      return false;
    }
    return true;
  };
  std::optional<Buffer> buffer_to_release;
  {
    const std::lock_guard lock(this->buffer_mutex_);
    if (this->pending_.buffer.has_value()) {
      // if the current buffer exists, is not uploaded yet, and is different
      // from newly attached buffer, current buffer will no longer be used
      if (is_valid_buffer(this->current_.buffer) &&
          !this->is_texture_uploaded_ &&
          this->current_.buffer != this->pending_.buffer) {
        buffer_to_release = this->current_.buffer;
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
    wl_buffer_send_release(buffer_to_release->resource);
  }

  if (const auto role = this->role_.lock()) {
    role->commit();
  }
}

void Surface::upload_texture_if_needed() {
  const std::lock_guard lock(this->buffer_mutex_);
  if (!this->current_.buffer.has_value() || this->is_texture_uploaded_) {
    return;
  }
  auto* shm_buffer      = this->current_.buffer->shm_buffer;
  this->texture_width_  = wl_shm_buffer_get_width(shm_buffer);
  this->texture_height_ = wl_shm_buffer_get_height(shm_buffer);
  auto* data            = wl_shm_buffer_get_data(shm_buffer);
  auto  format          = wl_shm_buffer_get_format(shm_buffer);
  auto  stride          = wl_shm_buffer_get_stride(shm_buffer);
  wl_shm_buffer_begin_access(shm_buffer);
  this->texture_.upload(
      data, format, this->texture_width_, this->texture_height_, stride);
  wl_shm_buffer_end_access(shm_buffer);
  this->current_.buffer->unref_shm_buffer();
  this->is_texture_uploaded_ = true;
  this->uploaded_buffer_     = this->current_.buffer;

  if (this->surface_node_.has_value()) {
    auto& scale = this->surface_node_.value()->geometry().scale();
    scale.x     = *this->texture_width_.to_world_unit();
    scale.y     = *this->texture_height_.to_world_unit();
  }
}
void Surface::finish_rendering() {
  std::optional<Buffer>       buffer;
  std::vector<wl_resource*>   frame_callbacks;
  std::optional<wl_resource*> release_callback;
  {
    const std::lock_guard lock(this->buffer_mutex_);
    buffer                 = this->uploaded_buffer_;
    this->uploaded_buffer_ = std::nullopt;
    frame_callbacks.swap(this->current_.frame_callbacks);
    release_callback                = this->current_.release_callback;
    this->current_.release_callback = std::nullopt;
  }
  if (!buffer.has_value() && frame_callbacks.empty() &&
      !release_callback.has_value()) {
    return;
  }

  const auto weak_surface = this->weak_from_this();
  server::get().enqueue(
      [weak_surface, buffer, frame_callbacks, release_callback]() {
        const auto surface = weak_surface.lock();
        if (!surface) {
          return;
        }
        if (buffer.has_value() && buffer->resource) {
          wl_buffer_send_release(buffer->resource);
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
    parent.value()->add_child(this->surface_node_.value());
  } else {
    server::get().scene.insert(this->surface_node_.value());
  }
  auto output = server::get().output_map.find(this->resource_->client);
  if (output != server::get().output_map.end()) {
    wl_surface_send_enter(this->resource_, output->second);
  }
}
void Surface::unmap() {
  if (!this->surface_node_.has_value()) {
    return;
  }
  server::get().scene.erase(this->surface_node_.value());
  this->surface_node_ = std::nullopt;
  auto output         = server::get().output_map.find(this->resource_->client);
  if (output != server::get().output_map.end()) {
    wl_surface_send_leave(this->resource_, output->second);
  }
}
}  // namespace yabe::core
