#pragma once

#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene-intersection.hpp"

namespace yabe::core::scene {
constexpr float kSceneNodeDistance = 0.7F;

class SceneNode : public std::enable_shared_from_this<SceneNode> {
 public:
  DISABLE_MOVE_AND_COPY(SceneNode);
  explicit SceneNode();
  virtual ~SceneNode() = default;

  void draw(const glm::mat4& vp_mat, render::DrawRectFunc& draw_rect,
      const SceneNode* parent, const glm::mat4& parent_world_transform);

  void add_child(std::shared_ptr<SceneNode> child);
  /// @return true if the child was erased
  bool try_erace_child(const std::shared_ptr<SceneNode>& child);
  /// @return true if the child was raised
  bool try_raise_child_to_front(const std::shared_ptr<SceneNode>& child);
  void move_absolute(const glm::vec3& pos);
  void move_relative(float polar, float azimuth);
  void look_at_camera();

  [[nodiscard]] std::optional<SceneIntersection> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction,
      const SceneNode* parent, const glm::mat4& parent_world_transform);

  [[nodiscard]] common::Geometry& geometry() {
    return this->geometry_;
  }
  [[nodiscard]] const common::Geometry& geometry() const {
    return this->geometry_;
  }
  [[nodiscard]] glm::mat4 world_transform() const;

 protected:
  // A node may need to determine its geometry before its world transform is
  // resolved, as SurfaceNode does by uploading a texture to obtain its size.
  virtual void prepare_for_draw()                                        = 0;
  virtual void draw_self(const glm::mat4& vp_mat,
      render::DrawRectFunc& draw_rect, const glm::mat4& world_transform) = 0;
  [[nodiscard]] virtual std::optional<SceneIntersection>
  check_intersection_self(const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) = 0;
  [[nodiscard]] virtual glm::mat4 resolve_world_transform(
      const SceneNode* parent, const glm::mat4& parent_world_transform) const;

 private:
  common::Geometry                      geometry_;
  std::list<std::shared_ptr<SceneNode>> children_;
  std::weak_ptr<SceneNode>              parent_;
};
}  // namespace yabe::core::scene
