#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "common/box.hpp"
#include "common/common.hpp"
#include "common/edge.hpp"
#include "common/thread-safety.hpp"
#include "scene-node.hpp"

namespace yabe::proto::xdg::shell::xdg_toplevel {
class XdgToplevel;
}

namespace yabe::core::scene {
class ResizeHandleNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(ResizeHandleNode);
  explicit ResizeHandleNode(
      const std::shared_ptr<proto::xdg::shell::xdg_toplevel::XdgToplevel>&
          toplevel);
  ~ResizeHandleNode() override = default;

  void motion(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos) override;
  void button(uint32_t button, bool is_pressed) override;
  void leave() override;

  [[nodiscard]] std::optional<common::Box2D> window_geometry();

  [[nodiscard]] std::shared_ptr<SceneNode> activation_target() override
      EXCLUDES(Scene::mutex);

 private:
  void prepare_for_draw() override REQUIRES(Scene::mutex);
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);

  common::Mutex              mutex_;
  common::Edge edges_        GUARDED_BY(mutex_);
  // percentage of intersected pos (window coordinate, not surface)
  //   - if x < 0, intersected pos is left side
  //   - if x > 1, intersected pos is right side
  glm::vec2 intersected_pos_ GUARDED_BY(mutex_);

  std::chrono::steady_clock::time_point alpha_updated_at_;
  float                                 alpha_ = 0.F;

  std::weak_ptr<proto::xdg::shell::xdg_toplevel::XdgToplevel> toplevel_;
};
}  // namespace yabe::core::scene
