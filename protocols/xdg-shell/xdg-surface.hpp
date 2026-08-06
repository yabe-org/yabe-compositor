#pragma once

#include <wayland-server-core.h>

#include "common/unit.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
class XdgSurface {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurface);
  XdgSurface(wl_resource* resource, std::weak_ptr<core::Surface>&& wl_surface);
  ~XdgSurface();

  void configure();
  void set_window_geometry(core::unit::Pixel x, core::unit::Pixel y,
      core::unit::Pixel width, core::unit::Pixel height);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::weak_ptr<core::Surface> wl_surface() const {
    return this->wl_surface_;
  }

  [[nodiscard]] core::unit::Pixel width() const {
    return this->width_;
  }
  [[nodiscard]] core::unit::Pixel height() const {
    return this->height_;
  }
  [[nodiscard]] core::unit::Pixel x() const {
    return this->x_;
  }
  [[nodiscard]] core::unit::Pixel y() const {
    return this->y_;
  }

 private:
  core::unit::Pixel            x_{0};
  core::unit::Pixel            y_{0};
  core::unit::Pixel            width_{0};
  core::unit::Pixel            height_{0};
  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> wl_surface_;
};

void create(wl_client* client, uint32_t id, wl_resource* wl_surface);
}  // namespace yabe::proto::xdg_shell::xdg_surface
