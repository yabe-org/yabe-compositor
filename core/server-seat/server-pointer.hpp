#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/surface.hpp"

namespace yabe::core::server_seat {
class ServerPointer {
 public:
  DISABLE_MOVE_AND_COPY(ServerPointer);
  explicit ServerPointer() = default;
  ~ServerPointer()         = default;

  void handle_pointer_move_relative(float dx, float dy);
  void handle_pointer_button(uint32_t button, bool is_pressed);
  void handle_pointer_axis(uint32_t axis, float value);

 private:
  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
                         get_focused_client_seat() const;
  /// @return true if the focused surface is changed
  bool                   set_focused_surface(std::weak_ptr<Surface> surface);
  std::weak_ptr<Surface> focused_surface_;
};
}  // namespace yabe::core::server_seat
