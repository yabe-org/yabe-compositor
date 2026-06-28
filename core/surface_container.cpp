#include "surface_container.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

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
}  // namespace yabe::core
