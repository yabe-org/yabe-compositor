#pragma once

#include <wayland-server.h>

#include "client-data-device.hpp"
#include "client-keyboard.hpp"
#include "client-pointer.hpp"
#include "client-ray.hpp"
#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientSeat {
 public:
  DISABLE_MOVE_AND_COPY(ClientSeat);
  explicit ClientSeat() = default;
  ~ClientSeat()         = default;

  ClientKeyboard   keyboard;
  ClientPointer    pointer;
  ClientDataDevice data_device;
  ClientRay        ray;
};
}  // namespace yabe::core::client_seat
