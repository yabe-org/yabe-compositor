#pragma once

#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "common/thread-safety.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene.hpp"

namespace yabe::core::client_seat {
class ClientSeat;
}

namespace yabe::core::scene {
constexpr float kSceneNodeDistance = 0.7F;

// The tree structure (children_ and parent_) is guarded by
// Scene::mutex. Functions that touch it require the lock, so take it
// with `const auto lock = Scene::mutex.lock();` before calling them
// outside of the scene graph traversal (e.g. draw(), check_intersection())
class SceneNode : public std::enable_shared_from_this<SceneNode> {
 public:
  DISABLE_MOVE_AND_COPY(SceneNode);
  explicit SceneNode();
  virtual ~SceneNode() = default;

  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const SceneNode* parent, const glm::mat4& parent_world_transform)
      REQUIRES(Scene::mutex);

  void add_child(std::shared_ptr<SceneNode> child) REQUIRES(Scene::mutex);
  /// @return true if the child was erased
  bool try_erace_child(const std::shared_ptr<SceneNode>& child)
      REQUIRES(Scene::mutex);
  /// @brief raise the descendant to the front among its siblings
  /// @return true if the descendant was found
  virtual bool try_raise_child_to_front(const std::shared_ptr<SceneNode>& child)
      REQUIRES(Scene::mutex);
  void move_absolute(const glm::vec3& pos);
  void move_relative(float polar, float azimuth);
  void look_at_camera();

  virtual void enter(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos);
  virtual void leave();
  virtual void motion(const glm::vec3& origin, const glm::vec3& direction,
      const glm::vec3& pos);
  virtual void button(uint32_t button, bool is_pressed);
  virtual void axis(uint32_t axis, float amount);
  virtual void frame();

  [[nodiscard]] std::optional<SceneIntersection> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction,
      const SceneNode* parent, const glm::mat4& parent_world_transform)
      REQUIRES(Scene::mutex);

  [[nodiscard]] const std::weak_ptr<SceneNode>& parent() const
      REQUIRES(Scene::mutex) {
    return this->parent_;
  }
  [[nodiscard]] common::Geometry& geometry() {
    return this->geometry_;
  }
  [[nodiscard]] const common::Geometry& geometry() const {
    return this->geometry_;
  }
  [[nodiscard]] glm::mat4 world_transform() const REQUIRES(Scene::mutex);

  [[nodiscard]] virtual wl_client* data_device_target() const {
    return nullptr;
  }
  /// the node that should become active when this node is clicked
  /// nullptr means that the current active node should be kept
  [[nodiscard]] virtual std::shared_ptr<SceneNode> activation_target()
      EXCLUDES(Scene::mutex) {
    return this->shared_from_this();
  }
  [[nodiscard]] bool has_child() const REQUIRES(Scene::mutex) {
    return !this->children_.empty();
  }

 protected:
  // A node may need to determine its geometry before its world transform is
  // resolved, as SurfaceNode does by uploading a texture to obtain its size.
  virtual void prepare_for_draw() REQUIRES(Scene::mutex)       = 0;
  virtual void draw_self(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& world_transform) REQUIRES(Scene::mutex) = 0;
  [[nodiscard]] virtual std::optional<SceneIntersection>
  check_intersection_self(const glm::vec3& origin, const glm::vec3& direction,
      const glm::mat4& world_transform) REQUIRES(Scene::mutex) = 0;
  [[nodiscard]] virtual glm::mat4 resolve_world_transform(
      const SceneNode* parent, const glm::mat4& parent_world_transform) const;
  /// called when a child is erased from this node
  virtual void handle_child_erased() REQUIRES(Scene::mutex);
  /// called when the size (geometry().scale()) of a child is changed
  virtual void handle_child_resized() REQUIRES(Scene::mutex);
  /// call this after changing geometry().scale() of this node
  void         notify_resized() REQUIRES(Scene::mutex);

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::list<std::shared_ptr<SceneNode>> children_ GUARDED_BY(Scene::mutex);

 private:
  common::Geometry                 geometry_;
  std::weak_ptr<SceneNode> parent_ GUARDED_BY(Scene::mutex);
};
}  // namespace yabe::core::scene
