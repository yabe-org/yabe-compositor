#pragma once

#include <wayland-server-core.h>

#include "common/box.hpp"
#include "common/unit.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
class XdgSurface {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurface);
  XdgSurface(wl_resource* resource, std::weak_ptr<core::Surface>&& wl_surface);
  ~XdgSurface();

  void commit();
  void configure();
  void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::weak_ptr<core::Surface> wl_surface() const {
    return this->wl_surface_;
  }

  [[nodiscard]] common::unit::Pixel width() const {
    return this->current_.width;
  }
  [[nodiscard]] common::unit::Pixel height() const {
    return this->current_.height;
  }
  [[nodiscard]] common::unit::Pixel x() const {
    return this->current_.x;
  }
  [[nodiscard]] common::unit::Pixel y() const {
    return this->current_.y;
  }

 private:
  common::Box2D                pending_, current_;
  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> wl_surface_;
};

void create(
    wl_client* client, int version, uint32_t id, wl_resource* wl_surface);
}  // namespace yabe::proto::xdg_shell::xdg_surface
