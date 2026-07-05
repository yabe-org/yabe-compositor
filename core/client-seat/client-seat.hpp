#pragma once

#include <wayland-server.h>

#include "client-pointer.hpp"
#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientSeat {
 public:
  DISABLE_MOVE_AND_COPY(ClientSeat);
  explicit ClientSeat() = default;
  ~ClientSeat()         = default;

  ClientPointer pointer;
};
}  // namespace yabe::core::client_seat
