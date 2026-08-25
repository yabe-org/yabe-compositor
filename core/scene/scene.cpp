#include "scene.hpp"

#include <mutex>
#include <ranges>

#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"

namespace yabe::core::scene {
void Scene::insert(std::shared_ptr<SceneNode>&& node) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  this->nodes_.push_back(std::move(node));
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
