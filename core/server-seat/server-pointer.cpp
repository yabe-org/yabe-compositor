#include "server-pointer.hpp"

#include <wayland-server.h>

#include <optional>

#include "core/server-seat/pointer-grab/default.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
ServerPointer::ServerPointer()
    : grab_(pointer_grab::DefaultPointerGrab::start()) {
}

void ServerPointer::unfocus_surface() {
  this->set_focused_scene_node({});
}
void ServerPointer::focus_pointed_surface() {
  const auto origin    = server::get().ray_cursor.origin();
  const auto direction = server::get().ray_cursor.direction();
  const auto intersection =
      server::get().scene.get_nearest_intersection(origin, direction);

  if (!intersection.has_value()) {
    this->set_focused_scene_node({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  auto       intersect_info = intersection.value().intersect_info;
  const auto scene_node     = intersection.value().node;
  if (!scene_node) {
    this->set_focused_scene_node({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  server::get().ray_cursor.set_length(intersect_info.distance);
  server::get().ray_cursor.set_cursor_rotation(
      scene_node->geometry().rotation());

  const auto is_focus_changed = this->set_focused_scene_node(scene_node);
  if (is_focus_changed) {
    scene_node->enter(origin, direction, intersect_info.pos);
  }
  scene_node->motion(origin, direction, intersect_info.pos);
  scene_node->frame();
}

void ServerPointer::start_grab(pointer_grab::PointerGrab* grab) {
  assert(grab);
  this->grab_ = grab;
}
void ServerPointer::end_grab() {
  this->grab_->cleanup();
  this->grab_ = pointer_grab::DefaultPointerGrab::start();
}

void ServerPointer::handle_pointer_move_relative(float dx, float dy) {
  this->grab_->move_relative(dx, dy);
}
void ServerPointer::handle_pointer_button(uint32_t button, bool is_pressed) {
  this->grab_->button(button, is_pressed);
}
// NOLINTBEGIN(readability-make-member-function-const): to keep consistency with
// other handle_pointer_* functions
void ServerPointer::handle_pointer_axis(uint32_t axis, float value) {
  const auto scene_node = this->focused_scene_node_.lock();
  if (!scene_node) {
    return;
  }
  scene_node->axis(axis, value);
  scene_node->frame();
}
// NOLINTEND(readability-make-member-function-const)

bool ServerPointer::set_focused_scene_node(std::shared_ptr<scene::SceneNode>
        scene_node) {  // NOLINT(performance-unnecessary-value-param)
  const auto current_focused_scene_node = this->focused_scene_node_.lock();
  if (current_focused_scene_node == scene_node) {
    return false;
  }
  if (current_focused_scene_node) {
    current_focused_scene_node->leave();
    current_focused_scene_node->frame();
  }

  this->focused_scene_node_ = scene_node;
  return true;
}
}  // namespace yabe::core::server_seat
