#include "scene.hpp"

#include <mutex>

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

void Scene::draw(render::DrawRectFunc& draw_rect) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (const auto& node : this->nodes_) {
    node->draw(draw_rect);
  }
}

std::optional<SceneIntersection> Scene::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  std::optional<SceneIntersection>  nearest;
  for (const auto& node : this->nodes_) {
    const auto intersection = node->check_intersection(origin, direction);
    if (!intersection.has_value()) {
      continue;
    }
    if (!nearest.has_value() || intersection->intersect_info.distance <
                                    nearest->intersect_info.distance) {
      nearest = intersection;
    }
  }
  return nearest;
}
}  // namespace yabe::core::scene
