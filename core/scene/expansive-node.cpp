#include "expansive-node.hpp"

#include <zwin-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/server.hpp"
#include "protocols/zwin/shell/expansive.hpp"

namespace yabe::core::scene {
ExpansiveNode::ExpansiveNode(
    const std::shared_ptr<proto::zwin::shell::Expansive>& expansive)
    : expansive_(expansive) {
}

void ExpansiveNode::enter(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& /*pos*/) {
  const auto expansive = this->expansive_.lock();
  auto       seat      = this->seat();
  if (!expansive || !seat.has_value()) {
    return;
  }
  auto virtual_object = expansive->virtual_object().lock();
  if (!virtual_object) {
    return;
  }
  seat.value()->ray.send_enter(virtual_object, origin, direction);
}
void ExpansiveNode::leave() {
  const auto expansive = this->expansive_.lock();
  auto       seat      = this->seat();
  if (!expansive || !seat.has_value()) {
    return;
  }
  auto virtual_object = expansive->virtual_object().lock();
  if (!virtual_object) {
    return;
  }
  seat.value()->ray.send_leave(virtual_object);
}
void ExpansiveNode::motion(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& /*pos*/) {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_motion(origin, direction);
}
void ExpansiveNode::button(uint32_t button, bool is_pressed) {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_button(button, is_pressed ?
                                            ZWN_RAY_BUTTON_STATE_PRESSED :
                                            ZWN_RAY_BUTTON_STATE_RELEASED);
}
void ExpansiveNode::axis(uint32_t axis, float amount) {
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
void ExpansiveNode::frame() {
  auto seat = this->seat();
  if (!seat.has_value()) {
    return;
  }
  seat.value()->ray.send_frame();
}

wl_client* ExpansiveNode::data_device_target() const {
  const auto expansive = this->expansive_.lock();
  if (!expansive) {
    return nullptr;
  }
  return expansive->client();
}

void ExpansiveNode::prepare_for_draw() {
}
void ExpansiveNode::draw_self(const glm::mat4& proj_mat,
    const glm::mat4& view_mat, const glm::mat4& world_transform) {
  const auto expansive = this->expansive_.lock();
  if (!expansive) {
    return;
  }
  expansive->draw(proj_mat, view_mat, world_transform);
}

std::optional<SceneIntersection> ExpansiveNode::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction,
    const glm::mat4& world_transform) {
  const auto expansive = this->expansive_.lock();
  if (!expansive || !expansive->region()) {
    return std::nullopt;
  }
  std::optional<float> min_distance;
  for (const auto& region : expansive->region().value().cuboids()) {
    const auto region_mat = world_transform *
                            glm::translate(glm::mat4(1.F), region.center) *
                            glm::toMat4(region.rotation);
    const auto result     = common::intersection::with_obb(
        origin, direction, region.half_size, region_mat);
    if (!result.has_value()) {
      continue;
    }
    if (!min_distance.has_value() || result.value() < min_distance.value()) {
      min_distance = result;
    }
  }
  if (!min_distance.has_value()) {
    return std::nullopt;
  }
  return SceneIntersection{
      .node           = this->shared_from_this(),
      .intersect_info = {.distance = min_distance.value(),
                         .pos                     = glm::vec3{0.F, 0.F, 0.F}},
  };
}

std::optional<std::shared_ptr<client_seat::ClientSeat>> ExpansiveNode::seat()
    const {
  const auto expansive = this->expansive_.lock();
  if (!expansive) {
    return std::nullopt;
  }
  return server::get().seat.get_client_seat(expansive->client());
}
}  // namespace yabe::core::scene
