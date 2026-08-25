#include "bounded-node.hpp"

#include "protocols/zwin/shell/bounded.hpp"

namespace yabe::core::scene {
BoundedNode::BoundedNode(
    const std::shared_ptr<proto::zwin::shell::Bounded>& bounded)
    : bounded_(bounded) {
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
  const auto model_mat = world_transform * this->geometry().transform_mat();
  bounded->draw(proj_mat, view_mat, model_mat);
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
  const auto model_mat = world_transform * this->geometry().transform_mat();
  const auto outer_distance = common::intersection::with_obb(
      origin, direction, bounded->half_size(), model_mat);
  if (!outer_distance.has_value()) {
    return std::nullopt;
  }

  std::optional<float> inner_min_distance;
  for (const auto& region : bounded->region().value().cuboids()) {
    const auto inner_mat = model_mat *
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
}  // namespace yabe::core::scene
