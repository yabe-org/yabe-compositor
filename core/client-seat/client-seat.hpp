#pragma once

#include <wayland-server.h>

#include "client-keyboard.hpp"
#include "client-pointer.hpp"
#include "common/common.hpp"
#include "core/client-seat/client-data-device.hpp"

namespace yabe::core::client_seat {
class ClientSeat {
 public:
  DISABLE_MOVE_AND_COPY(ClientSeat);
  explicit ClientSeat() = default;
  ~ClientSeat()         = default;

  ClientKeyboard   keyboard;
  ClientPointer    pointer;
  ClientDataDevice data_device;
};
}  // namespace yabe::core::client_seat
