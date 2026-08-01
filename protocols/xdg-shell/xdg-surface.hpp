#pragma once

#include <wayland-server-core.h>

#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
class XdgSurface {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurface);
  XdgSurface(wl_resource* resource, std::weak_ptr<core::Surface>&& wl_surface);
  ~XdgSurface() = default;

  void configure();

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::weak_ptr<core::Surface> wl_surface() const {
    return this->wl_surface_;
  }

 private:
  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> wl_surface_;
};

void create(wl_client* client, uint32_t id, wl_resource* wl_surface);
}  // namespace yabe::proto::xdg_shell::xdg_surface
