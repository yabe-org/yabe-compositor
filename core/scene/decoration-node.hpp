#pragma once

#include "common/common.hpp"
#include "core/render/texture.hpp"
#include "scene-node.hpp"

namespace yabe::core::scene {
class DecorationNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(DecorationNode);
  DecorationNode()           = default;
  ~DecorationNode() override = default;

  void reconfigure();

  void button(uint32_t button, bool is_pressed) override;

  [[nodiscard]] std::shared_ptr<SceneNode> activation_target() override;

 private:
  void prepare_for_draw() override;
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override;
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override;

  render::Texture texture_;
  bool            is_texture_uploaded_ = false;
};
}  // namespace yabe::core::scene
