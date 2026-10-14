#include "resize-handle-node.hpp"

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-protocol.h>

#include <chrono>
#include <glm/ext.hpp>
#include <optional>

#include "common/edge.hpp"
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
  const auto lock = this->mutex_.lock();
  this->edges_.reset();
  if (pos.x < 0.F) {
    this->edges_.set(common::Edge::LEFT);
  } else if (pos.x > 1.F) {
    this->edges_.set(common::Edge::RIGHT);
  }
  if (pos.y < 0.F) {
    this->edges_.set(common::Edge::TOP);
  } else if (pos.y > 1.F) {
    this->edges_.set(common::Edge::BOTTOM);
  }
  // convert window coordinate space
  // (0 -> 1, top to bottom) to (-0.5 -> +0.5, bottom to top)
  this->intersected_pos_ = {pos.x - 0.5F, -(pos.y - 0.5F)};
}
void ResizeHandleNode::button(uint32_t button, bool is_pressed) {
  if (button != BTN_LEFT || !is_pressed) {
    return;
  }
  const auto   toplevel = this->toplevel_.lock();
  // ResizePointerGrab calls leave(), which tries to acquire mutex!
  common::Edge edges;
  {
    const auto lock = this->mutex_.lock();
    edges           = this->edges_;
  }
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::ResizePointerGrab::start(
          toplevel, edges));
}
void ResizeHandleNode::leave() {
  const auto lock = this->mutex_.lock();
  this->edges_.reset();
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
  const auto parent = this->parent().lock();
  if (!parent) {
    return;
  }
  const common::unit::Pixel threshold{
      server::get().setting.config.get<"window", "resize_handle_threshold">()};
  // show handle at the middle of the threshold,
  // so there is no multiplier while actual interactable area is 2*thtreshold
  this->geometry().scale() = glm::vec3{
      parent->geometry().scale().x + *threshold.to_world_unit(),
      parent->geometry().scale().y + *threshold.to_world_unit(),
      1.F,
  };

  constexpr std::chrono::milliseconds kAnimationDurationMs{100};
  constexpr float                     kRate =
      1.F / std::chrono::duration<float>(kAnimationDurationMs).count();
  float direction = +1.F;
  {
    const auto lock = this->mutex_.lock();
    direction       = this->edges_.empty() ? -1.F : +1.F;
  }
  const auto now = std::chrono::steady_clock::now();
  const auto d   = std::chrono::duration<float>(now - this->alpha_updated_at_);
  this->alpha_ =
      std::clamp(this->alpha_ + (d.count() * kRate * direction), 0.F, 1.F);
  this->alpha_updated_at_ = now;
}
void ResizeHandleNode::draw_self(const glm::mat4& proj_mat,
    const glm::mat4& view_mat, const glm::mat4& world_transform) {
  const auto model_mat = proj_mat * view_mat * world_transform *
                         glm::scale(glm::mat4(1.F), this->geometry().scale());
  glm::vec2  pos{0.F};
  {
    const auto lock = this->mutex_.lock();
    pos             = this->intersected_pos_;
  }
  server::get().resize_handle_renderer.render(model_mat, pos, this->alpha_);
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
    return SceneIntersection{
        .node           = this->shared_from_this(),
        .intersect_info = {.distance = result->distance,
                           .pos = glm::vec3{result->u, 1.F - result->v, 0.F}},
    };
  }
  return std::nullopt;
}
}  // namespace yabe::core::scene
