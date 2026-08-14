#include "client-pointer.hpp"

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/server.hpp"
#include "wayland-server-core.h"

namespace yabe::core::client_seat {
void ClientPointer::add_pointer(wl_resource* resource) {
  LOG_DBG("add pointer#{}", wl_resource_get_id(resource));
  this->pointers_.push_back(resource);
}
void ClientPointer::remove_pointer(wl_resource* resource) {
  this->pointers_.remove(resource);
}

void ClientPointer::send_enter(
    wl_resource* surface_resource, float x, float y) {
  const auto serial  = server::get().next_serial();
  const auto fixed_x = wl_fixed_from_double(x);
  const auto fixed_y = wl_fixed_from_double(y);
  for (auto* resource : this->pointers_) {
    wl_pointer_send_enter(resource, serial, surface_resource, fixed_x, fixed_y);
  }
}

void ClientPointer::send_leave(wl_resource* surface_resource) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->pointers_) {
    wl_pointer_send_leave(resource, serial, surface_resource);
  }
}

void ClientPointer::send_motion(float x, float y) {
  const auto serial  = server::get().next_serial();
  const auto fixed_x = wl_fixed_from_double(x);
  const auto fixed_y = wl_fixed_from_double(y);
  for (auto* resource : this->pointers_) {
    wl_pointer_send_motion(resource, serial, fixed_x, fixed_y);
  }
}

void ClientPointer::send_button(uint32_t button, uint32_t state) {
  const auto serial = server::get().next_serial();
  const auto now    = common::now_msec();
  for (auto* resource : this->pointers_) {
    wl_pointer_send_button(resource, serial, now, button, state);
  }
}

void ClientPointer::send_axis(uint32_t axis, float value) {
  const auto serial      = server::get().next_serial();
  const auto fixed_value = wl_fixed_from_double(value);
  for (auto* resource : this->pointers_) {
    wl_pointer_send_axis(resource, serial, axis, fixed_value);
  }
}

void ClientPointer::send_frame() {
  for (auto* resource : this->pointers_) {
    if (wl_resource_get_version(resource) >= WL_POINTER_FRAME_SINCE_VERSION) {
      wl_pointer_send_frame(resource);
    }
  }
}
}  // namespace yabe::core::client_seat
