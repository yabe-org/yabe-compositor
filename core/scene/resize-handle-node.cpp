#include "resize-handle-node.hpp"

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-protocol.h>

#include <glm/ext.hpp>
#include <optional>

#include "common/unit.hpp"
#include "core/server-seat/pointer-grab/resize.hpp"
#include "core/server.hpp"

namespace yabe::core::scene {
ResizeHandleNode::ResizeHandleNode(
    const std::shared_ptr<proto::xdg::shell::xdg_toplevel::XdgToplevel>&
        toplevel)
    : toplevel_(toplevel) {
  this->geometry().pos() = glm::vec3{0.F};
}

void ResizeHandleNode::motion(const glm::vec3& /*origin*/,
    const glm::vec3& /*direction*/, const glm::vec3& pos) {
  uint32_t edge = 0;
  if (pos.x < 0.F) {
    edge |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
  } else if (pos.x > 0.F) {
    edge |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
  }
  if (pos.y < 0.F) {
    edge |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
  } else if (pos.y > 0.F) {
    edge |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
  }
  this->edges_.store(edge);
}
void ResizeHandleNode::button(uint32_t button, bool is_pressed) {
  if (button != BTN_LEFT || !is_pressed) {
    return;
  }
  const auto toplevel = this->toplevel_.lock();
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::ResizePointerGrab::start(
          toplevel, this->edges_.load()));
}
void ResizeHandleNode::leave() {
  this->edges_.store(0);
}

std::shared_ptr<SceneNode> ResizeHandleNode::activation_target() {
  std::shared_ptr<SceneNode> parent;
  {
    const auto lock = Scene::mutex.lock();
    parent          = this->parent().lock();
  }
  if (!parent) {
    return nullptr;
  }
  return parent->activation_target();
}

void ResizeHandleNode::prepare_for_draw() {
  // TODO
}
void ResizeHandleNode::draw_self(const glm::mat4& /*proj_mat*/,
    const glm::mat4& /*view_mat*/, const glm::mat4& /*world_transform*/) {
  // TODO
}

std::optional<SceneIntersection> ResizeHandleNode::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction,
    const glm::mat4& world_transform) {
  const common::unit::Pixel threshold{
      server::get().setting.config.get<"window", "resize_handle_threshold">()};
  if (*threshold <= 0) {
    return std::nullopt;
  }
  const auto parent = this->parent().lock();
  if (!parent) {
    return std::nullopt;
  }
  const auto scale    = parent->geometry().scale();
  const auto geom_mat = world_transform * glm::scale(glm::mat4(1.F), scale);
  auto result = common::intersection::with_surface(origin, direction, geom_mat);
  if (!result.has_value()) {
    return std::nullopt;
  }
  if ((0.F <= result->u && result->u <= 1.F) &&
      (0.F <= result->v && result->v <= 1.F)) {
    return std::nullopt;
  }
  const common::unit::WorldUnit x{scale.x * result->u};
  const common::unit::WorldUnit y{scale.y * (1.F - result->v)};
  const common::unit::WorldUnit width{scale.x};
  const common::unit::WorldUnit height{scale.y};
  // ignore if intersected pos is too far away from edges
  if (x < threshold.to_world_unit() * -1.F ||
      x > width + threshold.to_world_unit() ||
      y < threshold.to_world_unit() * -1.F ||
      y > height + threshold.to_world_unit()) {
    return std::nullopt;
  }
  const auto near_left   = *x < 0.F;
  const auto near_right  = x > width;
  const auto near_top    = *y < 0.F;
  const auto near_bottom = y > height;

  if (near_left || near_right || near_top || near_bottom) {
    glm::vec3 pos(0.F);
    if (near_left) {
      pos.x = -1.F;
    } else if (near_right) {
      pos.x = 1.F;
    }
    if (near_top) {
      pos.y = -1.F;
    } else if (near_bottom) {
      pos.y = 1.F;
    }
    return SceneIntersection{
        .node           = this->shared_from_this(),
        .intersect_info = {.distance = result->distance, .pos = pos},
    };
  }
  return std::nullopt;
}
}  // namespace yabe::core::scene
