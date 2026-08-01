#pragma once

#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "common/intersection.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/surface.hpp"

namespace yabe::core::scene {
class SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(SceneNode);
  explicit SceneNode(std::weak_ptr<Surface>&& surface);
  virtual ~SceneNode() = default;

  virtual void draw(render::DrawRectFunc& draw_rect) = 0;

  [[nodiscard]] std::optional<common::intersection::IntersectInfo>
  check_intersection(const glm::vec3& origin, const glm::vec3& direction);

  [[nodiscard]] std::weak_ptr<Surface> surface() const {
    return this->surface_;
  }

 protected:
  [[nodiscard]] virtual std::optional<common::intersection::IntersectInfo>
  check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) = 0;

  std::weak_ptr<Surface> surface_;

 private:
  std::list<std::shared_ptr<SceneNode>> children_;
};
}  // namespace yabe::core::scene
