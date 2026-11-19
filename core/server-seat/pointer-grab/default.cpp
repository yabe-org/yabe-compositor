#include "default.hpp"

#include <linux/input-event-codes.h>

#include <memory>

#include "core/scene/surface-node.hpp"
#include "core/server.hpp"
#include "implicit.hpp"
#include "locked.hpp"
#include "toplevel-selection.hpp"

namespace yabe::core::server_seat::pointer_grab {
DefaultPointerGrab DefaultPointerGrab::instance;

DefaultPointerGrab* DefaultPointerGrab::start() {
  return &instance;
}
bool DefaultPointerGrab::is_active() {
  return server::get().seat.pointer.is_active_grab(&instance);
}

void DefaultPointerGrab::cleanup() {
}

void DefaultPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(dy, dx);
  server::get().seat.pointer.focus_pointed_surface();
}

void DefaultPointerGrab::button(uint32_t button, bool is_pressed) {
  const auto pointer_focused_scene_node =
      server::get().seat.pointer.get_focused_scene_node().lock();
  if (is_pressed) {
    if (!pointer_focused_scene_node) {
      server::get().workspace.current_scene().set_active_node({});
    } else if (const auto target =
                   pointer_focused_scene_node->activation_target()) {
      server::get().workspace.current_scene().set_active_node(target);
    }
  }
  if (!pointer_focused_scene_node) {
    if (is_pressed && button == BTN_RIGHT &&
        server::get().setting.config.get<"surface_grouping", "enabled">()) {
      server::get().seat.pointer.start_grab(
          ToplevelSelectionPointerGrab::start());
    }
    return;
  }
  pointer_focused_scene_node->button(button, is_pressed);
  pointer_focused_scene_node->frame();
  const auto surface_node =
      std::dynamic_pointer_cast<scene::SurfaceNode>(pointer_focused_scene_node);
  if (!surface_node || !is_pressed) {
    return;
  }
  if (LockedPointerGrab::can_start_with(surface_node)) {
    server::get().seat.pointer.start_grab(
        LockedPointerGrab::start(surface_node));
    return;
  }
  server::get().seat.pointer.start_grab(
      ImplicitPointerGrab::start(surface_node, button));
}
}  // namespace yabe::core::server_seat::pointer_grab
