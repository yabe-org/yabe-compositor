#include "surface.hpp"

#include <sys/time.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>

#include "common/common.hpp"
#include "common/pos.hpp"
#include "core/server.hpp"
#include "wayland-server-core.h"

namespace yabe::core {
Surface::Surface(wl_resource* resource)
    : pos_{0.F, 0.F, -0.5F}, resource_(resource) {
  LOG_DBG("new wl_surface#{}", wl_resource_get_id(resource));
}
Surface::~Surface() {
  LOG_DBG("surface#{} deleted", wl_resource_get_id(this->resource_));
}

void Surface::set_callback(wl_resource* resource) {
  this->pending_.callback = resource;
}

void Surface::attach(wl_resource* resource) {
  this->pending_.buffer = resource;
}

void Surface::commit() {
  if (this->pending_.buffer.has_value()) {
    this->is_texture_uploaded_ = false;
    this->current_.buffer      = this->pending_.buffer;
    this->pending_.buffer      = std::nullopt;
  }

  if (this->pending_.callback.has_value()) {
    this->current_.callback = this->pending_.callback;
    this->pending_.callback = std::nullopt;
  }

  if (this->role_) {
    this->role_->commit();
  }
}

void Surface::upload_texture_if_needed() {
  if (!this->current_.buffer.has_value() || this->is_texture_uploaded_) {
    return;
  }
  this->is_texture_uploaded_       = true;
  auto*                 buffer     = this->current_.buffer.value();
  struct wl_shm_buffer* shm_buffer = wl_shm_buffer_get(buffer);
  if (!shm_buffer) {
    return;
  }
  this->width_  = wl_shm_buffer_get_width(shm_buffer);
  this->height_ = wl_shm_buffer_get_height(shm_buffer);
  auto* data    = wl_shm_buffer_get_data(shm_buffer);
  auto  format  = wl_shm_buffer_get_format(shm_buffer);
  auto  stride  = wl_shm_buffer_get_stride(shm_buffer);
  wl_shm_buffer_begin_access(shm_buffer);
  this->texture_.upload(data, format, this->width_, this->height_, stride);
  wl_shm_buffer_end_access(shm_buffer);
  wl_buffer_send_release(buffer);
}
void Surface::send_frame_done() {
  if (!this->current_.callback.has_value()) {
    return;
  }
  struct timeval now;
  gettimeofday(&now, nullptr);
  auto* callback = this->current_.callback.value();
  wl_callback_send_done(callback, (now.tv_sec * 1000) + (now.tv_usec / 1000));
  wl_resource_destroy(callback);
  this->current_.callback = std::nullopt;
}

void Surface::motion(const common::SurfacePos& /*pos*/) const {
}
void Surface::enter(const common::SurfacePos& /*pos*/) const {
}
void Surface::leave() const {
}
void Surface::frame() const {
}
void Surface::button(uint32_t /*time*/, uint32_t /*button*/,
    wl_pointer_button_state /*state*/) const {
}
void Surface::axis(
    uint32_t /*time*/, wl_pointer_axis /*axis*/, double /*valua*/) const {
}

void Surface::focus() {
  server::get().surfaces.raise_to_top(this);
  if (this->role_) {
    this->role_->set_activated(true);
  }
}
void Surface::unfocus() {
  if (this->role_) {
    this->role_->set_activated(false);
  }
}

glm::mat4 Surface::model_mat() const {
  constexpr float kPixelPerMeter = 2500.F;
  glm::mat4       model_mat(1.F);
  model_mat = glm::translate(model_mat, this->pos_) *
              glm::scale(model_mat, glm::vec3(this->width_ / kPixelPerMeter,
                                        this->height_ / kPixelPerMeter, 1.F));
  return model_mat;
}

Surface* Surface::from(wl_resource* wl_surface) {
  return static_cast<Surface*>(wl_resource_get_user_data(wl_surface));
}
}  // namespace yabe::core
