#pragma once

#include <wayland-server.h>

#include <list>

#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientRelativePointer {
 public:
  DISABLE_MOVE_AND_COPY(ClientRelativePointer);
  explicit ClientRelativePointer() = default;
  ~ClientRelativePointer()         = default;

  void add_relative_pointer(wl_resource* resource);
  void remove_relative_pointer(wl_resource* resource);

  void send_relative_motion(
      float dx, float dy, float dx_unaccel, float dy_unaccel);

 private:
  std::list<wl_resource*> relative_pointers_;
};
}  // namespace yabe::core::client_seat
