#include "scene-node.hpp"

#include <optional>

namespace yabe::core::scene {
SceneNode::SceneNode(std::weak_ptr<Surface>&& surface)
    : surface_(std::move(surface)) {
}

std::optional<common::intersection::IntersectInfo>
SceneNode::check_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  std::optional<common::intersection::IntersectInfo> nearest_intersection =
      std::nullopt;
  for (const auto& child : this->children_) {
    const auto child_intersection =
        child->check_intersection(origin, direction);
    if (!child_intersection.has_value()) {
      continue;
    }
    if (!nearest_intersection.has_value() ||
        child_intersection->distance < nearest_intersection->distance) {
      nearest_intersection = child_intersection;
    }
  }
  if (nearest_intersection.has_value()) {
    return nearest_intersection;
  }
  return this->check_intersection_self(origin, direction);
}
}  // namespace yabe::core::scene
