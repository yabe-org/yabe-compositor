#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "core/surface.hpp"

namespace yabe::core::server_seat {
class ServerPointer {
 public:
  DISABLE_MOVE_AND_COPY(ServerPointer);
  explicit ServerPointer();
  ~ServerPointer() = default;

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
                                              get_focused_client_seat() const;
  [[nodiscard]] const std::weak_ptr<Surface>& get_focused_surface() const {
    return this->focused_surface_;
  }
  void focus_pointed_surface();
  /// @param surface nullable
  void set_surface_as_cursor(
      wl_resource* surface, int32_t hotspot_x, int32_t hotspot_y);
  [[nodiscard]] const std::weak_ptr<Surface>& get_cursor_surface() const {
    return this->cursor_surface_;
  }

  void start_grab(pointer_grab::PointerGrab* grab);
  void end_grab();

  void handle_pointer_move_relative(float dx, float dy);
  void handle_pointer_button(uint32_t button, bool is_pressed);
  void handle_pointer_axis(uint32_t axis, float value);

 private:
  /// @return true if the focused surface is changed
  bool set_focused_surface(std::weak_ptr<Surface> surface);

  std::weak_ptr<Surface> focused_surface_;
  std::weak_ptr<Surface> cursor_surface_;

  pointer_grab::PointerGrab* grab_;
};
}  // namespace yabe::core::server_seat
