#pragma once

#include <memory>

#include "scene-node.hpp"

namespace yabe::proto::zwin::shell {
class Expansive;
}

namespace yabe::core::scene {
class ExpansiveNode : public SceneNode {
 public:
  DISABLE_MOVE_AND_COPY(ExpansiveNode);
  explicit ExpansiveNode(
      const std::shared_ptr<proto::zwin::shell::Expansive>& expansive);
  ~ExpansiveNode() override = default;

  void enter(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos) override;
  void leave() override;
  void motion(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos) override;
  void button(uint32_t button, bool is_pressed) override;
  void axis(uint32_t axis, float amount) override;
  void frame() override;

  [[nodiscard]] wl_client* data_device_target() const override;

 private:
  void prepare_for_draw() override REQUIRES(Scene::mutex);
  void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);
  [[nodiscard]] std::optional<SceneIntersection> check_intersection_self(
      const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) override REQUIRES(Scene::mutex);

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>> seat()
      const;

  // weak reference to avoid circular reference. See SurfaceNode for details
  std::weak_ptr<proto::zwin::shell::Expansive> expansive_;
};
}  // namespace yabe::core::scene
