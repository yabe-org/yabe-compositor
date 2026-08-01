#include "scene.hpp"

#include <algorithm>

namespace yabe::core::scene {
void Scene::insert(std::shared_ptr<SceneNode>&& node) {
  this->nodes_.push_back(std::move(node));
}

void Scene::erase(SceneNode* node) {
  this->nodes_.remove_if(
      [node](const std::shared_ptr<SceneNode>& candidate) {
        return candidate.get() == node;
      });
}

void Scene::draw(render::DrawRectFunc& draw_rect) {
  for (const auto& node : this->nodes_) {
    node->draw(draw_rect);
  }
}

std::optional<
    std::pair<std::weak_ptr<Surface>, common::intersection::IntersectInfo>>
Scene::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  std::optional<
      std::pair<std::weak_ptr<Surface>, common::intersection::IntersectInfo>>
      nearest;
  for (const auto& node : this->nodes_) {
    const auto intersection = node->check_intersection(origin, direction);
    if (!intersection.has_value()) {
      continue;
    }
    if (!nearest.has_value() ||
        intersection->distance < nearest->second.distance) {
      nearest = std::make_pair(node->surface(), *intersection);
    }
  }
  return nearest;
}
}  // namespace yabe::core::scene
