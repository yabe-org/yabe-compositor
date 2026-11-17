#pragma once

#include <wayland-server-core.h>

#include "common/box.hpp"
#include "common/unit.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg::shell::xdg_surface {
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

  [[nodiscard]] common::Box2D window_geometry() const {
    if (this->current_window_geometry_) {
      return *this->current_window_geometry_;
    }
    if (const auto surface = this->wl_surface_.lock()) {
      return common::Box2D{
          .width  = surface->texture_width(),
          .height = surface->texture_height(),
      };
    }
    return {};
  }

  [[nodiscard]] common::unit::Pixel width() const {
    return this->window_geometry().width;
  }
  [[nodiscard]] common::unit::Pixel height() const {
    return this->window_geometry().height;
  }
  [[nodiscard]] common::unit::Pixel x() const {
    return this->window_geometry().x;
  }
  [[nodiscard]] common::unit::Pixel y() const {
    return this->window_geometry().y;
  }

 private:
  std::optional<common::Box2D> pending_window_geometry_,
      current_window_geometry_;
  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> wl_surface_;
};

void create(
    wl_client* client, int version, uint32_t id, wl_resource* wl_surface);
}  // namespace yabe::proto::xdg::shell::xdg_surface
