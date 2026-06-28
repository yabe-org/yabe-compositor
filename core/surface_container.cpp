#include "surface_container.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <optional>
#include <ranges>
#include <utility>

#include "common/pos.hpp"

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

std::optional<std::pair<Surface*, common::SurfacePos>>
SurfaceContainer::get_intersected_at(const common::ScreenPos& pos) {
  // last surface is the topmost
  for (auto& it : std::ranges::reverse_view(this->surfaces_)) {
    auto* surface = it.get();
    if (surface->type() == SurfaceType::CURSOR) {
      continue;
    }
    if (pos.x >= surface->x() && pos.x < surface->x() + surface->width() &&
        pos.y >= surface->y() && pos.y < surface->y() + surface->height()) {
      return std::make_pair(surface,
          common::SurfacePos{pos.x - surface->x(), pos.y - surface->y()});
    }
  }
  return std::nullopt;
}

void SurfaceContainer::raise_to_top(Surface* surface) {
  auto it = surface->get_self_iter();
  this->surfaces_.splice(this->surfaces_.end(), this->surfaces_, it);
}
}  // namespace yabe::core
