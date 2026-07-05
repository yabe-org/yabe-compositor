#pragma once

#include <wayland-server-core.h>

#include <functional>
#include <list>

#include "common/common.hpp"
#include "surface.hpp"

namespace yabe::core {
class SurfaceContainer {
 public:
  DISABLE_MOVE_AND_COPY(SurfaceContainer);
  explicit SurfaceContainer() = default;
  ~SurfaceContainer()         = default;

  Surface* create_new_surface(wl_resource* wl_surface);
  void     erase(wl_resource* wl_surface);

  void raise_to_top(Surface* surface);
  std::optional<std::pair<std::weak_ptr<Surface>, IntersectInfo>>
  get_nearest_intersection(const glm::vec3& origin, const glm::vec3& direction);

  void foreach (const std::function<void(Surface&)>& func) {
    for (auto& surface_ptr : this->surfaces_) {
      func(*surface_ptr);
    }
  }

 private:
  // front surface is the backmost
  std::list<std::shared_ptr<Surface>> surfaces_;
};
}  // namespace yabe::core
