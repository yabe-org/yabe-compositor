#pragma once

#include "common/common.hpp"
#include "common/intersection.hpp"
#include "core/scene/scene-node.hpp"
#include "core/surface.hpp"

namespace yabe::core::scene {
constexpr float kSurfaceDistance = 0.7F;

class Toplevel : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(Toplevel);
  explicit Toplevel(std::weak_ptr<core::Surface>&& surface);
  ~Toplevel() override = default;

  void draw(render::DrawRectFunc& draw_rect) override;

 private:
  [[nodiscard]] std::optional<common::intersection::IntersectInfo>
  check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) override;
};
}  // namespace yabe::core::scene
