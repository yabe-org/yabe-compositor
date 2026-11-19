#pragma once

#include <wayland-server.h>

#include "client-data-device.hpp"
#include "client-keyboard.hpp"
#include "client-pointer.hpp"
#include "client-ray.hpp"
#include "client-relative-pointer.hpp"
#include "client-text-input.hpp"
#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientSeat {
 public:
  DISABLE_MOVE_AND_COPY(ClientSeat);
  explicit ClientSeat() = default;
  ~ClientSeat()         = default;

  ClientKeyboard        keyboard;
  ClientPointer         pointer;
  ClientRelativePointer relative_pointer;
  ClientDataDevice      data_device;
  ClientRay             ray;
  ClientTextInput       text_input;
};
}  // namespace yabe::core::client_seat
