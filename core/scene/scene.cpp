#include "scene.hpp"

#include <mutex>
#include <ranges>

#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server.hpp"

namespace yabe::core::scene {
void Scene::insert(std::shared_ptr<SceneNode>&& node) {
  auto insert_pos = this->nodes_.end();
  if (this->active_node_.lock()) {
    // active node is always at the end of the list,
    // so if it exists, new node should be inserted before it
    if (insert_pos != this->nodes_.begin()) {
      --insert_pos;
    }
  } else {
    this->set_active_node(node);
  }
  const std::lock_guard<std::mutex> lock(this->mutex_);
  this->nodes_.emplace(insert_pos, std::move(node));
}

void Scene::erase(const std::shared_ptr<SceneNode>& node) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (auto& it : this->nodes_) {
    if (it == node) {
      this->nodes_.remove(it);
      return;
    }
    if (it->try_erace_child(node)) {
      return;
    }
  }
}

void Scene::raise_to_front(const std::shared_ptr<SceneNode>& node) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  auto it = std::find(this->nodes_.begin(), this->nodes_.end(), node);
  if (it != this->nodes_.end()) {
    this->nodes_.splice(this->nodes_.end(), this->nodes_, it);
    return;
  }
  for (auto& it : this->nodes_) {
    if (it->try_raise_child_to_front(node)) {
      return;
    }
  }
}

void Scene::set_active_node(const std::shared_ptr<SceneNode>& node) {
  // do not lock mutex here because this sends Wayland related events only
  if (node == this->active_node_.lock()) {
    return;
  }
  if (const auto surface_node = std::dynamic_pointer_cast<SurfaceNode>(node)) {
    if (const auto toplevel = surface_node->surface()->get_toplevel()) {
      toplevel->set_activated(false);
    }
  }
  this->active_node_ = node;
  this->focus_to_active_node();
}
void Scene::focus_to_active_node() {
  const auto node = this->active_node_.lock();
  if (!node) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  this->raise_to_front(node);

  auto* data_device_target = node->data_device_target();
  if (data_device_target) {
    server::get().seat.data_device.send_selection(data_device_target);
  }

  const auto surface_node = std::dynamic_pointer_cast<SurfaceNode>(node);
  if (!surface_node) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  const auto toplevel = surface_node->surface()->get_toplevel();
  if (!toplevel) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  server::get().seat.keyboard.set_focused_surface(toplevel->surface().lock());
  toplevel->set_activated(true);
}

void Scene::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (const auto& node : this->nodes_) {
    node->draw(proj_mat, view_mat, nullptr, glm::mat4(1.F));
  }
}

std::optional<SceneIntersection> Scene::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (const auto& node : this->nodes_ | std::views::reverse) {
    const auto intersection =
        node->check_intersection(origin, direction, nullptr, glm::mat4(1.F));
    if (intersection.has_value()) {
      return intersection;
    }
  }
  return std::nullopt;
}
}  // namespace yabe::core::scene
