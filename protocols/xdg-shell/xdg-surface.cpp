#include "xdg-surface.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/server.hpp"
#include "protocols/xdg-shell/xdg-popup.hpp"
#include "protocols/xdg-shell/xdg-positioner.hpp"
#include "xdg-toplevel.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
XdgSurface::XdgSurface(
    wl_resource* resource, std::weak_ptr<core::Surface>&& wl_surface)
    : resource_(resource), wl_surface_(std::move(wl_surface)) {
  LOG_DBG("xdg_surface#{}: created", wl_resource_get_id(resource));
}
XdgSurface::~XdgSurface() {
  LOG_DBG("xdg_surface#{}: destroyed", wl_resource_get_id(this->resource_));
}

void XdgSurface::commit() {
  this->current_ = this->pending_;
}
void XdgSurface::configure() {
  xdg_surface_send_configure(
      this->resource_, core::server::get().next_serial());
}
void XdgSurface::set_window_geometry(
    int32_t x, int32_t y, int32_t width, int32_t height) {
  this->pending_ = common::Box2D{
      .x      = common::unit::Pixel(x),
      .y      = common::unit::Pixel(y),
      .width  = common::unit::Pixel(width),
      .height = common::unit::Pixel(height),
  };
}

namespace {
constexpr struct xdg_surface_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_toplevel =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          auto self = core::server::get().xdg_surface_map.at(resource);
          xdg_toplevel::create(
              client, wl_resource_get_version(resource), id, self);
        },
    .get_popup =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* parent, wl_resource* positioner) {
          auto parent_surface =
              core::server::get().xdg_surface_map.find(parent);
          if (parent_surface == core::server::get().xdg_surface_map.end()) {
            wl_client_post_implementation_error(
                wl_resource_get_client(resource),
                "xdg_popup requested for unknown parent surface");
            return;
          }
          auto self = core::server::get().xdg_surface_map.at(resource);
          xdg_popup::create(client, wl_resource_get_version(resource), id, self,
              parent_surface->second, xdg_positioner::get(positioner));
        },
    .set_window_geometry =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          auto self = core::server::get().xdg_surface_map.at(resource);
          self->set_window_geometry(x, y, width, height);
        },
    .ack_configure =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            uint32_t /*serial*/) {
          // TODO
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().xdg_surface_map.erase(resource);
}
}  // namespace

void create(
    wl_client* client, int version, uint32_t id, wl_resource* wl_surface) {
  auto surface = core::server::get().surface_map.find(wl_surface);
  if (surface == core::server::get().surface_map.end()) {
    wl_client_post_implementation_error(
        client, "xdg_surface requested for unknown wl_surface");
    return;
  }
  wl_resource* resource =
      wl_resource_create(client, &xdg_surface_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  core::server::get().xdg_surface_map.emplace(resource,
      std::make_shared<XdgSurface>(resource, std::weak_ptr(surface->second)));
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
}
}  // namespace yabe::proto::xdg_shell::xdg_surface
