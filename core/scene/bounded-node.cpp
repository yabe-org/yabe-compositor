#include "bounded-node.hpp"

#include <zwin-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "protocols/zwin/shell/bounded.hpp"

namespace yabe::core::scene {
BoundedNode::BoundedNode(
    const std::shared_ptr<proto::zwin::shell::Bounded>& bounded)
    : bounded_(bounded) {
  this->geometry().scale() = bounded->half_size() * 2.F;
}

void BoundedNode::enter(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& /*pos*/) {
  const auto bounded = this->bounded();
  auto       seat    = this->seat();
  if (!bounded || !seat.has_value()) {
    return;
  }
  auto virtual_object = bounded->virtual_object().lock();
  if (!virtual_object) {
    return;
  }
  seat.value()->ray.send_enter(virtual_object, origin, direction);
}
void BoundedNode::leave() {
  const auto bounded = this->bounded();
  auto       seat    = this->seat();
  if (!bounded || !seat.has_value()) {
    return;
  }
  auto virtual_object = bounded->virtual_object().lock();
  if (!virtual_object) {
    return;
  }
  seat.value()->ray.send_leave(virtual_object);
}
void BoundedNode::motion(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& /*pos*/) {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_motion(origin, direction);
}
void BoundedNode::button(uint32_t button, bool is_pressed) {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_button(button, is_pressed ?
                                            ZWN_RAY_BUTTON_STATE_PRESSED :
                                            ZWN_RAY_BUTTON_STATE_RELEASED);
}
void BoundedNode::axis(uint32_t axis, float amount) {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  switch (axis) {
    case 0:
      seat.value()->ray.send_axis(ZWN_RAY_AXIS_VERTICAL_SCROLL, amount);
      break;
    case 1:
      seat.value()->ray.send_axis(ZWN_RAY_AXIS_HORIZONTAL_SCROLL, amount);
      break;
    default:
      break;
  }
}
void BoundedNode::frame() {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_frame();
}

wl_client* BoundedNode::data_device_target() const {
  const auto bounded = this->bounded_.lock();
  if (!bounded) {
    return nullptr;
  }
  return bounded->client();
}

void BoundedNode::prepare_for_draw() {
  // nothing to do
}
void BoundedNode::draw_self(const glm::mat4& proj_mat,
    const glm::mat4& view_mat, const glm::mat4& world_transform) {
  const auto bounded = this->bounded_.lock();
  if (!bounded) {
    return;
  }
  bounded->draw(proj_mat, view_mat, world_transform);
}

[[nodiscard]] std::optional<SceneIntersection>
BoundedNode::check_intersection_self(const glm::vec3& origin,
    const glm::vec3& direction, const glm::mat4& world_transform) {
  const auto bounded = this->bounded_.lock();
  if (!bounded) {
    return std::nullopt;
  }
  if (!bounded->region()) {
    return std::nullopt;
  }
  const auto outer_distance = common::intersection::with_obb(
      origin, direction, bounded->half_size(), world_transform);
  if (!outer_distance.has_value()) {
    return std::nullopt;
  }

  std::optional<float> inner_min_distance;
  for (const auto& region : bounded->region().value().cuboids()) {
    const auto inner_mat = world_transform *
                           glm::translate(glm::mat4(1.F), region.center) *
                           glm::toMat4(region.rotation);
    const auto result    = common::intersection::with_obb(
        origin, direction, region.half_size, inner_mat);
    if (!result.has_value()) {
      continue;
    }
    if (!inner_min_distance.has_value() ||
        result.value() < inner_min_distance.value()) {
      inner_min_distance = result;
    }
  }
  if (!inner_min_distance.has_value()) {
    return std::nullopt;
  }
  return SceneIntersection{
      .node           = this->shared_from_this(),
      .intersect_info = {.distance = inner_min_distance.value(),
                         .pos                     = glm::vec3{0.F, 0.F, 0.F}},
  };
}

std::optional<std::shared_ptr<client_seat::ClientSeat>> BoundedNode::seat()
    const {
  const auto bounded = this->bounded_.lock();
  if (!bounded) {
    return std::nullopt;
  }
  return server::get().seat.get_client_seat(bounded->client());
}
}  // namespace yabe::core::scene
