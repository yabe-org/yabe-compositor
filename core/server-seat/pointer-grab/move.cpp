#include "move.hpp"

#include <linux/input-event-codes.h>

#include <memory>

#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
MovePointerGrab* MovePointerGrab::start(
    std::weak_ptr<scene::SceneNode>&& scene_node) {
  static MovePointerGrab instance;
  server::get().seat.pointer.unfocus_surface();
  server::get().workspace.current_scene().raise_to_front(scene_node.lock());
  instance.moving_scene_node_ = std::move(scene_node);
  return &instance;
}

void MovePointerGrab::cleanup() {
  server::get().seat.pointer.focus_pointed_surface();
}

void MovePointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(dy, dx);

  const auto intersection =
      server::get().workspace.current_scene().get_nearest_intersection(
          server::get().ray_cursor.origin(),
          server::get().ray_cursor.direction());
  if (intersection) {
    server::get().ray_cursor.set_length(
        intersection.value().intersect_info.distance);
    server::get().ray_cursor.set_cursor_rotation(
        intersection.value().node->geometry().rotation());
  }

  const auto toplevel = this->moving_scene_node_.lock();
  if (!toplevel) {
    server::get().seat.pointer.end_grab();
    return;
  }
  bool does_toplevel_have_parent = false;
  {
    const auto lock           = scene::Scene::mutex.lock();
    does_toplevel_have_parent = toplevel->parent().lock() != nullptr;
  }
  if (does_toplevel_have_parent) {
    toplevel->detach_from_parent();
  }
  toplevel->move_relative(dy, dx);
  toplevel->look_at_camera();
}

void MovePointerGrab::button(uint32_t button, bool is_pressed) {
  if (button == BTN_LEFT && !is_pressed) {
    server::get().seat.pointer.end_grab();
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
