#pragma once

#include <memory>

#include "common/common.hpp"
#include "common/intersection.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core {
class Surface;
}
namespace yabe::core::scene {
constexpr float kSurfaceDistance = 0.7F;

class SurfaceNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(SurfaceNode);
  explicit SurfaceNode(std::shared_ptr<core::Surface>&& surface);
  ~SurfaceNode() override = default;

  [[nodiscard]] std::shared_ptr<Surface> surface() const {
    return this->surface_;
  }

 private:
  void draw_self(render::DrawRectFunc& draw_rect) override;
  [[nodiscard]] std::optional<common::intersection::IntersectInfo>
  check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) override;

  std::shared_ptr<core::Surface> surface_;
};
}  // namespace yabe::core::scene
