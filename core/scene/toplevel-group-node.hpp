#pragma once

#include "core/render/texture.hpp"
#include "scene-node.hpp"

namespace yabe::core::scene {
class ToplevelGroupNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(ToplevelGroupNode);
  ToplevelGroupNode()           = default;
  ~ToplevelGroupNode() override = default;

  // parent node pass weak_ptr of them by using weak_from_this(), which is not
  // available in constructor, so we need to append children after construction
  void init(std::list<std::shared_ptr<SceneNode>>& node);
  void reconfigure();

  void leave() override;
  void motion(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  void prepare_for_draw() override;
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override;
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override;
  void handle_child_erased() override;

  bool is_cursor_on_decoration_ = false;

  render::Texture bg_texture_;
  bool            is_texture_uploaded_ = false;
};
}  // namespace yabe::core::scene
