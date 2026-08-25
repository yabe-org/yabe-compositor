#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <mutex>

#include "common/common.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core::scene {
class Scene {
 public:
  DISABLE_MOVE_AND_COPY(Scene);
  Scene()  = default;
  ~Scene() = default;

  void insert(std::shared_ptr<SceneNode>&& node);
  void erase(const std::shared_ptr<SceneNode>& node);
  void raise_to_front(const std::shared_ptr<SceneNode>& node);

  void draw(const glm::mat4& vp_mat);

  std::optional<SceneIntersection> get_nearest_intersection(
      const glm::vec3& origin, const glm::vec3& direction);

 private:
  std::list<std::shared_ptr<SceneNode>> nodes_;
  std::mutex                            mutex_;
};
}  // namespace yabe::core::scene
