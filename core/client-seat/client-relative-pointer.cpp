#include "client-relative-pointer.hpp"

#include <relative-pointer-protocol.h>

#include "common/common.hpp"
#include "common/time.hpp"

namespace yabe::core::client_seat {
void ClientRelativePointer::add_relative_pointer(wl_resource* resource) {
  LOG_DBG("add relative_pointer#{}", wl_resource_get_id(resource));
  this->relative_pointers_.push_back(resource);
}
void ClientRelativePointer::remove_relative_pointer(wl_resource* resource) {
  this->relative_pointers_.remove(resource);
}

void ClientRelativePointer::send_relative_motion(
    float dx, float dy, float dx_unaccel, float dy_unaccel) {
  const auto now_usec         = common::now_usec();
  const auto utime_hi         = now_usec >> 32;
  const auto utime_lo         = now_usec & 0xFFFF'FFFF;
  const auto fixed_dx         = wl_fixed_from_double(dx);
  const auto fixed_dy         = wl_fixed_from_double(dy);
  const auto fixed_dx_unaccel = wl_fixed_from_double(dx_unaccel);
  const auto fixed_dy_unaccel = wl_fixed_from_double(dy_unaccel);
  for (auto* resource : this->relative_pointers_) {
    zwp_relative_pointer_v1_send_relative_motion(resource, utime_hi, utime_lo,
        fixed_dx, fixed_dy, fixed_dx_unaccel, fixed_dy_unaccel);
  }
}
}  // namespace yabe::core::client_seat
