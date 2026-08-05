#pragma once

#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene-intersection.hpp"

namespace yabe::core::scene {
class SceneNode : public std::enable_shared_from_this<SceneNode> {
 public:
  DISABLE_MOVE_AND_COPY(SceneNode);
  explicit SceneNode(const Geometry& geometry);
  virtual ~SceneNode() = default;

  void draw(render::DrawRectFunc& draw_rect);

  void add_child(std::shared_ptr<SceneNode> child);
  /// @return true if the child was erased
  bool try_erace_child(const std::shared_ptr<SceneNode>& child);
  /// @return true if the child was raised
  bool try_raise_child_to_front(const std::shared_ptr<SceneNode>& child);
  void move_absolute(const glm::vec3& pos);
  void move_relative(float polar, float azimuth);

  [[nodiscard]] std::optional<SceneIntersection> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction);

  [[nodiscard]] Geometry& geometry() {
    return this->geometry_;
  }

 protected:
  virtual void draw_self(render::DrawRectFunc& draw_rect) = 0;
  [[nodiscard]] virtual std::optional<SceneIntersection>
  check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) = 0;

 private:
  void look_at_camera();

  Geometry                              geometry_;
  std::list<std::shared_ptr<SceneNode>> children_;
};
}  // namespace yabe::core::scene
