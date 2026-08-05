#include "scene.hpp"

#include "core/scene/scene-node.hpp"

namespace yabe::core::scene {
void Scene::insert(std::shared_ptr<SceneNode>&& node) {
  this->nodes_.push_back(std::move(node));
}

void Scene::erase(const std::shared_ptr<SceneNode>& node) {
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
  for (const auto& node : this->nodes_) {
    node->draw(draw_rect);
  }
}

std::optional<
    std::pair<std::weak_ptr<SceneNode>, common::intersection::IntersectInfo>>
Scene::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  std::optional<
      std::pair<std::weak_ptr<SceneNode>, common::intersection::IntersectInfo>>
      nearest;
  for (const auto& node : this->nodes_) {
    const auto intersection = node->check_intersection(origin, direction);
    if (!intersection.has_value()) {
      continue;
    }
    if (!nearest.has_value() ||
        intersection->distance < nearest->second.distance) {
      nearest = std::make_pair(node, *intersection);
    }
  }
  return nearest;
}
}  // namespace yabe::core::scene
