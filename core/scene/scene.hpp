#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "common/common.hpp"
#include "common/intersection.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core::scene {
class Scene {
 public:
  DISABLE_MOVE_AND_COPY(Scene);
  Scene()  = default;
  ~Scene() = default;

  void insert(std::shared_ptr<SceneNode>&& node);
  void erase(const std::shared_ptr<SceneNode>& node);

  void draw(render::DrawRectFunc& draw_rect);

  std::optional<
      std::pair<std::weak_ptr<SceneNode>, common::intersection::IntersectInfo>>
  get_nearest_intersection(const glm::vec3& origin, const glm::vec3& direction);

 private:
  std::list<std::shared_ptr<SceneNode>> nodes_;
};
}  // namespace yabe::core::scene
