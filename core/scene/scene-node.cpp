#include "scene-node.hpp"

#include <algorithm>
#include <optional>

#include "core/scene/scene-intersection.hpp"

namespace yabe::core::scene {
SceneNode::SceneNode()
    : geometry_(common::Geometry(glm::vec3{0.F, 0.F, -kSceneNodeDistance})) {
}

// NOLINTNEXTLINE(misc-no-recursion): ???
void SceneNode::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const SceneNode* parent, const glm::mat4& parent_world_transform) {
  this->prepare_for_draw();
  const auto world_transform =
      this->resolve_world_transform(parent, parent_world_transform);
  this->draw_self(proj_mat, view_mat, world_transform);
  for (const auto& child : this->children_) {
    child->draw(proj_mat, view_mat, this, world_transform);
  }
}

void SceneNode::add_child(std::shared_ptr<SceneNode> child) {
  child->parent_ = this->weak_from_this();
  this->children_.push_back(std::move(child));
}
// NOLINTNEXTLINE(misc-no-recursion): ???
bool SceneNode::try_erace_child(const std::shared_ptr<SceneNode>& child) {
  return std::ranges::any_of(this->children_, [&](const auto& it) {
    if (it == child) {
      it->parent_.reset();
      this->children_.remove(it);
      return true;
    }
    return it->try_erace_child(child);
  });
}
bool SceneNode::try_raise_child_to_front(
    const std::shared_ptr<SceneNode>& child) {
  auto it = std::find(this->children_.begin(), this->children_.end(), child);
  if (it == this->children_.end()) {
    return false;
  }
  this->children_.splice(this->children_.end(), this->children_, it);
  return true;
}

void SceneNode::move_absolute(const glm::vec3& pos) {
  this->geometry_.move_absolute(pos);
}
void SceneNode::move_relative(float polar, float azimuth) {
  this->geometry_.move_relative(azimuth, polar, 0.F);
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

// NOLINTNEXTLINE(misc-no-recursion): ???
glm::mat4 SceneNode::world_transform() const {
  const auto parent = this->parent_.lock();
  if (!parent) {
    return this->resolve_world_transform(nullptr, glm::mat4(1.F));
  }
  return this->resolve_world_transform(parent.get(), parent->world_transform());
}

glm::mat4 SceneNode::resolve_world_transform(const SceneNode* /*parent*/,
    const glm::mat4& parent_world_transform) const {
  return parent_world_transform * this->geometry_.transform_mat();
}

std::optional<SceneIntersection>
SceneNode::check_intersection(  // NOLINT(misc-no-recursion): ???
    const glm::vec3& origin, const glm::vec3& direction,
    const SceneNode* parent, const glm::mat4& parent_world_transform) {
  const auto world_transform =
      this->resolve_world_transform(parent, parent_world_transform);
  std::optional<SceneIntersection> nearest_intersected_child;
  for (const auto& child : this->children_) {
    const auto child_intersection =
        child->check_intersection(origin, direction, this, world_transform);
    if (!child_intersection.has_value()) {
      continue;
    }
    if (!nearest_intersected_child.has_value() ||
        child_intersection->intersect_info.distance <=
            nearest_intersected_child->intersect_info.distance) {
      nearest_intersected_child = child_intersection;
    }
  }
  if (nearest_intersected_child.has_value()) {
    return nearest_intersected_child;
  }
  return this->check_intersection_self(origin, direction, world_transform);
}
}  // namespace yabe::core::scene
