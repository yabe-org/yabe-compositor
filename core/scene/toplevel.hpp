#pragma once

#include "common/common.hpp"
#include "common/geometry.hpp"
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

  void move_absolute(const glm::vec3& pos);
  void move_relative(float polar, float azimuth);

  [[nodiscard]] glm::mat4        model_mat();
  [[nodiscard]] const glm::vec3& position() {
    return this->geometry_.pos();
  }

  [[nodiscard]] std::weak_ptr<Surface> surface() const {
    return this->surface_;
  }

 private:
  Geometry geometry_;

  [[nodiscard]] std::optional<common::intersection::IntersectInfo>
  check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction) override;
};
}  // namespace yabe::core::scene
