#pragma once

#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <optional>
#include <span>

#include "common/common.hpp"
#include "common/thread-safety.hpp"
#include "core/scene/scene-intersection.hpp"

namespace yabe::core {
struct RayState;
}
namespace yabe::core::scene {
// SceneNode includes this header in order to refer to Scene::mutex
// in its annotations, so this header must not include scene-node.hpp
class SceneNode;

class Scene {
 public:
  DISABLE_MOVE_AND_COPY(Scene);
  Scene()  = default;
  ~Scene() = default;

  /// shared by all Scenes because SceneNode cannot know
  /// which Scene it belongs to
  static inline common::Mutex mutex;

  void insert(std::shared_ptr<SceneNode>&& node) EXCLUDES(mutex);
  void erase(const std::shared_ptr<SceneNode>& node) EXCLUDES(mutex);
  void raise_to_front(const std::shared_ptr<SceneNode>& node) EXCLUDES(mutex);
  void set_active_node(const std::shared_ptr<SceneNode>& node) EXCLUDES(mutex);
  void focus_to_active_node() EXCLUDES(mutex);

  std::list<std::shared_ptr<SceneNode>> extract_lasso_selected_surface_nodes(
      std::span<RayState> lasso_points) EXCLUDES(mutex);

  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat)
      EXCLUDES(mutex);

  std::optional<SceneIntersection> get_nearest_intersection(
      const glm::vec3& origin, const glm::vec3& direction) EXCLUDES(mutex);

 private:
  std::list<std::shared_ptr<SceneNode>> nodes_ GUARDED_BY(mutex);
  std::weak_ptr<SceneNode>                     active_node_;
};
}  // namespace yabe::core::scene
