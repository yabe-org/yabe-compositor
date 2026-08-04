#include "surface-container.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"

namespace yabe::core {
std::weak_ptr<Surface> SurfaceContainer::create_new_surface(
    wl_resource* wl_surface) {
  auto surface = std::make_shared<Surface>(wl_surface);
  this->surfaces_.emplace_back(surface);
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
  auto surface = std::find_if(this->surfaces_.begin(), this->surfaces_.end(),
      [wl_surface](const std::shared_ptr<Surface>& s) {
        return s->resource() == wl_surface;
      });
  if (surface == this->surfaces_.end()) {
    return;
  }
  surface->get()->unmap();
  this->surfaces_.erase(surface);
}

void SurfaceContainer::raise_to_top(wl_resource* wl_surface) {
  const auto surface = std::find_if(this->surfaces_.begin(),
      this->surfaces_.end(), [wl_surface](const auto& candidate) {
        return candidate->resource() == wl_surface;
      });
  if (surface == this->surfaces_.end()) {
    return;
  }
  this->surfaces_.splice(this->surfaces_.end(), this->surfaces_, surface);
}
}  // namespace yabe::core
