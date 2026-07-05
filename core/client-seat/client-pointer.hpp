#pragma once

#include <wayland-server.h>

#include <list>

#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientPointer {
 public:
  DISABLE_MOVE_AND_COPY(ClientPointer);
  explicit ClientPointer() = default;
  ~ClientPointer()         = default;

  void add_pointer(wl_resource* resource);
  void remove_pointer(wl_resource* resource);

  void send_enter(wl_resource* surface_resource, float x, float y);
  void send_leave(wl_resource* surface_resource);
  void send_motion(float x, float y);
  void send_button(uint32_t button, uint32_t state);
  void send_axis(uint32_t axis, float value);
  void send_frame();

 private:
  std::list<wl_resource*> pointers_;
};
}  // namespace yabe::core::client_seat
