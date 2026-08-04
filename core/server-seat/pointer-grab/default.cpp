#include "default.hpp"

#include <memory>

#include "core/server-seat/pointer-grab/implicit.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
DefaultPointerGrab* DefaultPointerGrab::start() {
  static DefaultPointerGrab instance;
  return &instance;
}

void DefaultPointerGrab::cleanup() {
}

void DefaultPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(-dy, -dx);
  server::get().seat.pointer.focus_pointed_surface();
}

void DefaultPointerGrab::button(uint32_t button, bool is_pressed) {
  const auto pointer_focused_surface_node =
      server::get().seat.pointer.get_focused_surface_node().lock();
  if (!pointer_focused_surface_node) {
    return;
  }
  if (is_pressed) {
    server::get().seat.keyboard.set_focused_surface(
        pointer_focused_surface_node->surface());
  }
  const auto& client_seat =
      server::get().seat.pointer.get_focused_client_seat();
  if (!client_seat) {
    return;
  }
  client_seat.value()->pointer.send_button(button, is_pressed);
  client_seat.value()->pointer.send_frame();
  if (is_pressed) {
    server::get().seat.pointer.start_grab(
        ImplicitPointerGrab::start(pointer_focused_surface_node, button));
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
