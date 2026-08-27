#include "default.hpp"

#include <memory>

#include "core/scene/surface-node.hpp"
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
  if (is_pressed) {
    server::get().workspace.current_scene().set_active_node(
        pointer_focused_scene_node);
  }
  if (!pointer_focused_scene_node) {
    return;
  }
  pointer_focused_scene_node->button(button, is_pressed);
  pointer_focused_scene_node->frame();
  const auto surface_node =
      std::dynamic_pointer_cast<scene::SurfaceNode>(pointer_focused_scene_node);
  if (is_pressed && surface_node) {
    server::get().seat.pointer.start_grab(
        ImplicitPointerGrab::start(surface_node, button));
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
