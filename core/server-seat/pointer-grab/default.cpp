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
  const auto pointer_focused_scene_node =
      server::get().seat.pointer.get_focused_scene_node().lock();
  if (!pointer_focused_scene_node) {
    server::get().seat.set_active_toplevel({});
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  const auto surface_node =
      std::dynamic_pointer_cast<scene::SurfaceNode>(pointer_focused_scene_node);
  if (is_pressed) {
    auto* client = pointer_focused_scene_node->data_device_target();
    if (client) {
      // data_device.selection must be sent immediately before keyboard focus
      server::get().seat.data_device.send_selection(client);
    }
    if (surface_node) {
      const auto toplevel = surface_node->surface()->get_toplevel();
      if (toplevel) {
        server::get().seat.keyboard.set_focused_surface(toplevel->surface());
        server::get().seat.set_active_toplevel(toplevel);
      }
    }
  }
  pointer_focused_scene_node->button(button, is_pressed);
  pointer_focused_scene_node->frame();
  if (is_pressed && surface_node) {
    server::get().seat.pointer.start_grab(
        ImplicitPointerGrab::start(surface_node, button));
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
