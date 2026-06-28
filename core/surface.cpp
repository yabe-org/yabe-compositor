#include "surface.hpp"

#include <bits/types/struct_timeval.h>
#include <sys/time.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "common/common.hpp"
#include "common/pos.hpp"
#include "core/server.hpp"

namespace yabe::core {
Surface::Surface(wl_resource* resource) : pos_{0, 0}, resource_(resource) {
  LOG_DBG("new wl_surface#{}", wl_resource_get_id(resource));
}
Surface::~Surface() {
  LOG_DBG("surface#{} deleted", wl_resource_get_id(this->resource_));
}

void Surface::set_callback(wl_resource* resource) {
  this->callback_ = resource;
}

void Surface::attach(wl_resource* resource) {
  this->buffer_ = resource;
}

void Surface::commit() {
  if (this->buffer_.has_value()) {
    auto*                 buffer     = this->buffer_.value();
    struct wl_shm_buffer* shm_buffer = wl_shm_buffer_get(buffer);
    if (!shm_buffer) {
      return;
    }
    this->width_  = wl_shm_buffer_get_width(shm_buffer);
    this->height_ = wl_shm_buffer_get_height(shm_buffer);
    // TODO
  }

  if (this->callback_.has_value()) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    auto* callback = this->callback_.value();
    wl_callback_send_done(callback, (now.tv_sec * 1000) + (now.tv_usec / 1000));
    wl_resource_destroy(callback);
    this->callback_ = std::nullopt;
  }
  if (this->role_) {
    this->role_->commit();
  }
}

void Surface::move_absolute(const common::ScreenPos& pos) {
  this->pos_ = pos;
}
void Surface::move_relative(double dx, double dy) {
  this->pos_.x += dx;
  this->pos_.y += dy;
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

Surface* Surface::from(wl_resource* wl_surface) {
  return static_cast<Surface*>(wl_resource_get_user_data(wl_surface));
}
}  // namespace yabe::core
