#include "implicit.hpp"

#include <linux/input-event-codes.h>

#include "common/intersection.hpp"
#include "common/unit.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
ImplicitPointerGrab* ImplicitPointerGrab::start(
    const std::shared_ptr<scene::SurfaceNode>& focused_surface_node,
    uint32_t                                   button) {
  static ImplicitPointerGrab instance;
  instance.pressed_buttons_.clear();
  instance.pressed_buttons_.insert(button);
  instance.focused_surface_node_ = focused_surface_node;
  return &instance;
}

void ImplicitPointerGrab::cleanup() {
  server::get().seat.pointer.focus_pointed_surface();
}

void ImplicitPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(-dy, -dx);
  // do not unfocus because it is implicit grab!

  auto scene_node = this->focused_surface_node_.lock();
  if (!scene_node) {
    server::get().seat.pointer.end_grab();
    return;
  }
  auto      geom_mat          = scene_node->model_mat();
  glm::vec3 vert_left_bottom  = geom_mat * glm::vec4(-0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = geom_mat * glm::vec4(+0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_left_top     = geom_mat * glm::vec4(-0.5F, +0.5F, 0.F, 1.F);
  auto      result            = common::intersection::with_surface(
      server::get().ray_cursor.origin(), server::get().ray_cursor.direction(),
      vert_left_bottom, vert_right_bottom, vert_left_top);
  if (!result.has_value()) {
    server::get().seat.pointer.end_grab();
    return;
  }

  const common::unit::WorldUnit surface_width{scene_node->geometry().scale().x};
  const common::unit::WorldUnit surface_height{
      scene_node->geometry().scale().y};
  const glm::vec3 pos{
      static_cast<float>(*surface_width.to_pixel()) * result->u,
      static_cast<float>(*surface_height.to_pixel()) * (1.F - result->v),
      0.F,
  };
  scene_node->motion(server::get().ray_cursor.origin(),
      server::get().ray_cursor.direction(), pos);
  scene_node->frame();
}

void ImplicitPointerGrab::button(uint32_t button, bool is_pressed) {
  const auto scene_node = this->focused_surface_node_.lock();
  if (!scene_node) {
    server::get().seat.pointer.end_grab();
    return;
  }
  scene_node->button(button, is_pressed);
  scene_node->frame();

  if (is_pressed) {
    pressed_buttons_.insert(button);
  } else {
    pressed_buttons_.erase(button);
  }

  if (pressed_buttons_.empty()) {
    server::get().seat.pointer.end_grab();
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
