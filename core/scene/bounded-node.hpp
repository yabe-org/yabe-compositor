#pragma once

#include "scene-node.hpp"

namespace yabe::proto::zwin::shell {
class Bounded;
}

namespace yabe::core::scene {
class BoundedNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(BoundedNode);
  explicit BoundedNode(
      const std::shared_ptr<proto::zwin::shell::Bounded>& bounded);
  ~BoundedNode() override = default;

 private:
  void prepare_for_draw() override;
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override;
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override;

  // weak reference to avoid circular reference. See SurfaceNode for details
  std::weak_ptr<proto::zwin::shell::Bounded> bounded_;
};
}  // namespace yabe::core::scene
