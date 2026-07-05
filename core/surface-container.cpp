#include "surface-container.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <memory>
#include <utility>

namespace yabe::core {
Surface* SurfaceContainer::create_new_surface(wl_resource* wl_surface) {
  auto  surface = std::make_unique<Surface>(wl_surface);
  auto* p       = surface.get();
  auto  it = this->surfaces_.emplace(this->surfaces_.end(), std::move(surface));
  p->set_iter(it);
  return p;
}

void SurfaceContainer::erase(wl_resource* wl_surface) {
  auto it = Surface::from(wl_surface)->get_self_iter();
  this->surfaces_.erase(it);
}

void SurfaceContainer::raise_to_top(Surface* surface) {
  auto it = surface->get_self_iter();
  this->surfaces_.splice(this->surfaces_.end(), this->surfaces_, it);
}

std::optional<std::pair<std::weak_ptr<Surface>, IntersectInfo>>
SurfaceContainer::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  std::optional<std::pair<std::weak_ptr<Surface>, IntersectInfo>> nearest =
      std::nullopt;
  for (const auto& surface : this->surfaces_) {
    auto intersection = surface->check_intersection(origin, direction);
    if (!intersection.has_value()) {
      continue;
    }
    if (!nearest.has_value() ||
        intersection->distance < nearest->second.distance) {
      nearest = std::make_pair(surface, intersection.value());
    };
  }
  return nearest;
}
}  // namespace yabe::core
