#include "surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "common/intersection.hpp"
#include "common/time.hpp"
#include "core/server.hpp"
#include "wayland-server-core.h"

namespace yabe::core {
Surface::Surface(wl_resource* resource)
    : geometry_({0.F, 0.F, -kSurfaceDistance}), resource_(resource) {
  LOG_DBG("new wl_surface#{}", wl_resource_get_id(resource));
}
Surface::~Surface() {
  LOG_DBG("surface#{} deleted", wl_resource_get_id(this->resource_));
}

void Surface::set_callback(wl_resource* resource) {
  const std::lock_guard lock(this->buffer_mutex_);
  this->pending_.callback = resource;
}

void Surface::attach(wl_resource* resource) {
  const std::lock_guard lock(this->buffer_mutex_);
  this->pending_.buffer = resource;
}

void Surface::commit() {
  std::optional<wl_resource*> buffer_to_release;
  {
    const std::lock_guard lock(this->buffer_mutex_);
    if (this->pending_.buffer.has_value()) {
      // if the current buffer exists, is not uploaded yet, and is different
      // from newly attached buffer, current buffer will no longer be used
      bool is_current_buffer_valid = this->current_.buffer.has_value() &&
                                     *this->current_.buffer != nullptr;
      if (!this->is_texture_uploaded_ && is_current_buffer_valid &&
          this->current_.buffer != this->pending_.buffer) {
        buffer_to_release = this->current_.buffer;
      }
      this->is_texture_uploaded_ = false;
      this->current_.buffer      = this->pending_.buffer;
      this->pending_.buffer      = std::nullopt;
    }

    if (this->pending_.callback.has_value()) {
      this->current_.callback = this->pending_.callback;
      this->pending_.callback = std::nullopt;
    }
  }

  if (buffer_to_release.has_value()) {
    wl_buffer_send_release(buffer_to_release.value());
  }

  if (this->role_) {
    this->role_->commit();
  }
}

void Surface::upload_texture_if_needed() {
  const std::lock_guard lock(this->buffer_mutex_);
  if (!this->current_.buffer.has_value() || this->is_texture_uploaded_) {
    return;
  }
  this->is_texture_uploaded_       = true;
  auto*                 buffer     = this->current_.buffer.value();
  struct wl_shm_buffer* shm_buffer = wl_shm_buffer_get(buffer);
  if (!shm_buffer) {
    return;
  }
  this->texture_width_.value  = wl_shm_buffer_get_width(shm_buffer);
  this->texture_height_.value = wl_shm_buffer_get_height(shm_buffer);
  auto* data                  = wl_shm_buffer_get_data(shm_buffer);
  auto  format                = wl_shm_buffer_get_format(shm_buffer);
  auto  stride                = wl_shm_buffer_get_stride(shm_buffer);
  wl_shm_buffer_begin_access(shm_buffer);
  this->texture_.upload(
      data, format, this->texture_width_, this->texture_height_, stride);
  wl_shm_buffer_end_access(shm_buffer);
  this->uploaded_buffer_ = buffer;
}
void Surface::finish_rendering() {
  std::optional<wl_resource*> buffer;
  std::optional<wl_resource*> callback;
  {
    const std::lock_guard lock(this->buffer_mutex_);
    buffer                  = this->uploaded_buffer_;
    this->uploaded_buffer_  = std::nullopt;
    callback                = this->current_.callback;
    this->current_.callback = std::nullopt;
  }
  if (!buffer.has_value() && !callback.has_value()) {
    return;
  }

  const auto weak_surface = this->weak_from_this();
  server::get().enqueue([weak_surface, buffer, callback]() {
    const auto surface = weak_surface.lock();
    if (!surface) {
      return;
    }
    if (buffer.has_value()) {
      wl_buffer_send_release(buffer.value());
    }
    if (!callback.has_value()) {
      return;
    }
    wl_callback_send_done(callback.value(), common::now_msec());
    wl_resource_destroy(callback.value());
  });
}

void Surface::move_absolute(const glm::vec3& pos) {
  this->geometry_.move_absolute(pos);
}
void Surface::move_relative(float polar, float azimuth) {
  this->geometry_.move_relative(azimuth, polar, 0.F);
}

void Surface::focus() {
  server::get().surfaces.raise_to_top(this->resource_);
  if (this->role_) {
    this->role_->set_activated(true);
  }
}
void Surface::unfocus() {
  if (this->role_) {
    this->role_->set_activated(false);
  }
}

glm::mat4 Surface::model_mat() {
  glm::mat4 model_mat(1.F);
  model_mat =
      glm::translate(model_mat, this->geometry_.pos()) *
      glm::scale(model_mat, glm::vec3(*this->texture_width_.to_world_unit(),
                                *this->texture_height_.to_world_unit(), 0.F));
  return model_mat;
}

std::optional<IntersectInfo> Surface::check_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  if (!this->current_.buffer.has_value() ||
      this->type_ == SurfaceType::CURSOR) {
    return std::nullopt;
  }
  auto      geom_mat          = this->model_mat();
  glm::vec3 vert_left_bottom  = geom_mat * glm::vec4(-0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = geom_mat * glm::vec4(+0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_left_top     = geom_mat * glm::vec4(-0.5F, +0.5F, 0.F, 1.F);
  auto      result            = common::intersection::with_surface(
      origin, direction, vert_left_bottom, vert_right_bottom, vert_left_top);
  if (!result.has_value()) {
    return std::nullopt;
  }
  return IntersectInfo{
      .distance = result->distance,
      .pos{static_cast<float>(*this->texture_width_) * result->u,
           static_cast<float>(*this->texture_height_) * (1.F - result->v), 0.F}
  };
}
}  // namespace yabe::core
