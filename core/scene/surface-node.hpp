#pragma once

#include <memory>

#include "common/common.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core {
class Surface;
}
namespace yabe::core::scene {
constexpr float kSurfaceDistance = 0.7F;

// Surface owns this and erases this from Scene on destruction
class SurfaceNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(SurfaceNode);
  explicit SurfaceNode(std::weak_ptr<core::Surface>&& surface);
  ~SurfaceNode() override = default;

  [[nodiscard]] std::shared_ptr<Surface> surface() const {
    return this->surface_.lock();
  }

 private:
  void draw_self(render::DrawRectFunc& draw_rect) override;
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) override;

  // weak reference to avoid circular reference
  // Surface always valid while SurfaceNode exists
  // (note that Surface owner is Server::surface_map)
  std::weak_ptr<core::Surface> surface_;
};
}  // namespace yabe::core::scene
