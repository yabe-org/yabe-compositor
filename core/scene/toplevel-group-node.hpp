#pragma once

#include <memory>
#include <ranges>

#include "core/render/texture.hpp"
#include "core/scene/decoration-node.hpp"
#include "scene-node.hpp"

namespace yabe::core::scene {
class ToplevelGroupNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(ToplevelGroupNode);
  ToplevelGroupNode()           = default;
  ~ToplevelGroupNode() override = default;

  // parent node pass weak_ptr of them by using weak_from_this(), which is not
  // available in constructor, so we need to append children after construction
  void init(std::list<std::shared_ptr<SceneNode>>& node) REQUIRES(Scene::mutex);
  void reconfigure() REQUIRES(Scene::mutex);

  [[nodiscard]] std::shared_ptr<SceneNode> activation_target() override
      EXCLUDES(Scene::mutex) {
    return nullptr;
  }

 private:
  void prepare_for_draw() override REQUIRES(Scene::mutex);
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);
  void handle_child_erased() override REQUIRES(Scene::mutex);
  void handle_child_resized() override REQUIRES(Scene::mutex);
  bool try_raise_child_to_front(
      const std::shared_ptr<SceneNode>& child) override REQUIRES(Scene::mutex);

  [[nodiscard]] auto toplevels() const REQUIRES(Scene::mutex) {
    return this->children_ | std::views::filter([](const auto& c) {
      return !std::dynamic_pointer_cast<DecorationNode>(c);
    });
  }

  std::shared_ptr<DecorationNode> decoration_;
  render::Texture                 bg_texture_;
  bool                            is_texture_uploaded_ = false;
};
}  // namespace yabe::core::scene
