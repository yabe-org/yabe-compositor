#include "scene-node.hpp"

#include <optional>

namespace yabe::core::scene {
SceneNode::SceneNode(const Geometry& geometry) : geometry_(geometry) {
}

// NOLINTNEXTLINE(misc-no-recursion): ???
void SceneNode::draw(render::DrawRectFunc& draw_rect) {
  this->draw_self(draw_rect);
  for (const auto& child : this->children_) {
    child->draw(draw_rect);
  }
}

void SceneNode::add_child(std::shared_ptr<SceneNode> child) {
  this->children_.push_back(std::move(child));
}
bool SceneNode::try_erace_child(const std::shared_ptr<SceneNode>& child) {
  return this->children_.remove(child) > 0;
}

void SceneNode::move_absolute(const glm::vec3& pos) {
  this->geometry_.move_absolute(pos);
  this->look_at_camera();
}
void SceneNode::move_relative(float polar, float azimuth) {
  this->geometry_.move_relative(azimuth, polar, 0.F);
  this->look_at_camera();
}
void SceneNode::look_at_camera() {
  const auto pos                  = this->geometry_.pos();
  const auto distance_from_y_axis = glm::length(glm::vec2(pos.x, pos.z));
  // if the distance from Y axis is zero, yaw can't be determined
  const auto yaw = distance_from_y_axis > 0.F ? glm::atan(-pos.x, -pos.z) : 0.F;
  const auto pitch = glm::atan(pos.y, distance_from_y_axis);
  auto rotation    = glm::rotate(glm::mat4(1.F), yaw, glm::vec3(0.F, 1.F, 0.F));
  rotation         = glm::rotate(rotation, pitch, glm::vec3(1.F, 0.F, 0.F));
  this->geometry().rotation() = rotation;
}

std::optional<common::intersection::IntersectInfo>
SceneNode::check_intersection(  // NOLINT(misc-no-recursion): ???
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
