#pragma once

#include <cstdint>
#include <memory>

#include "common/common.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core {
class Surface;
}
namespace yabe::core::scene {
// Surface owns this and erases this from Scene on destruction
class SurfaceNode : public SceneNode {
 public:
  enum class Anchor : uint8_t {
    CENTER,
    TOP_LEFT,
  };

  DISABLE_MOVE_AND_COPY(SurfaceNode);
  explicit SurfaceNode(
      std::weak_ptr<core::Surface>&& surface, Anchor anchor = Anchor::CENTER);
  ~SurfaceNode() override = default;

  [[nodiscard]] std::shared_ptr<Surface> surface() const {
    return this->surface_.lock();
  }
  [[nodiscard]] glm::mat4 model_mat() const;

 private:
  void prepare_for_draw() override;
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override;
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override;
  [[nodiscard]] glm::mat4 resolve_world_transform(const SceneNode* parent,
      const glm::mat4& parent_world_transform) const override;

  // weak reference to avoid circular reference
  // Surface always valid while SurfaceNode exists
  // (note that Surface owner is Server::resources)
  std::weak_ptr<core::Surface> surface_;
  Anchor                       anchor_;
};
}  // namespace yabe::core::scene
