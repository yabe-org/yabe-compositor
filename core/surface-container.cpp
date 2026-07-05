#include "surface-container.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <memory>
#include <utility>

namespace yabe::core {
std::weak_ptr<Surface> SurfaceContainer::create_new_surface(
    wl_resource* wl_surface) {
  auto surface = std::make_shared<Surface>(wl_surface);
  auto it      = this->surfaces_.emplace(this->surfaces_.end(), surface);
  surface->set_iter(it);
  return surface;
}

std::weak_ptr<Surface> SurfaceContainer::get(wl_resource* wl_surface) {
  auto surface = std::find_if(this->surfaces_.begin(), this->surfaces_.end(),
      [wl_surface](const std::shared_ptr<Surface>& s) {
        return s->resource() == wl_surface;
      });
  if (surface == this->surfaces_.end()) {
    return {};
  }
  return *surface;
}

void SurfaceContainer::erase(wl_resource* wl_surface) {
  auto surface = this->get(wl_surface).lock();
  if (surface) {
    this->surfaces_.erase(surface->get_self_iter());
  }
}

void SurfaceContainer::raise_to_top(wl_resource* wl_surface) {
  auto surface = this->get(wl_surface).lock();
  if (surface) {
    this->surfaces_.splice(
        this->surfaces_.end(), this->surfaces_, surface->get_self_iter());
  }
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
