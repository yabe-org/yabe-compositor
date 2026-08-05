#include "xdg-surface.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "protocols/xdg-shell/xdg-popup.hpp"
#include "protocols/xdg-shell/xdg-positioner.hpp"
#include "xdg-toplevel.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
XdgSurface::XdgSurface(
    wl_resource* resource, std::weak_ptr<core::Surface>&& wl_surface)
    : resource_(resource), wl_surface_(std::move(wl_surface)) {
}

void XdgSurface::configure() {
  xdg_surface_send_configure(
      this->resource_, core::server::get().next_serial());
}
void XdgSurface::set_window_geometry(core::unit::Pixel x, core::unit::Pixel y,
    core::unit::Pixel width, core::unit::Pixel height) {
  this->x_      = x;
  this->y_      = y;
  this->width_  = width;
  this->height_ = height;
}

namespace {
void destroy(wl_client* /* client */, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_toplevel(wl_client* client, wl_resource* resource, uint32_t id) {
  auto self = core::server::get().xdg_surface_map.at(resource);
  xdg_toplevel::create(client, id, self);
}
void get_popup(wl_client* client, wl_resource* resource, uint32_t id,
    wl_resource* parent, wl_resource* positioner) {
  auto parent_surface = core::server::get().xdg_surface_map.find(parent);
  if (parent_surface == core::server::get().xdg_surface_map.end()) {
    wl_client_post_implementation_error(wl_resource_get_client(resource),
        "xdg_popup requested for unknown parent surface");
    return;
  }
  auto self = core::server::get().xdg_surface_map.at(resource);
  xdg_popup::create(client, id, self, parent_surface->second,
      xdg_positioner::get(positioner));
}
void set_window_geometry(wl_client* /* client */, wl_resource* resource,
    int32_t x, int32_t y, int32_t width, int32_t height) {
  // TODO
  auto self = core::server::get().xdg_surface_map.at(resource);
  self->set_window_geometry({x}, {y}, {width}, {height});
}
void ack_configure(wl_client* /* client */, wl_resource* /* resource */,
    uint32_t /* serial */) {
  // TODO
}
constexpr struct xdg_surface_interface kImpl = {
    .destroy             = destroy,
    .get_toplevel        = get_toplevel,
    .get_popup           = get_popup,
    .set_window_geometry = set_window_geometry,
    .ack_configure       = ack_configure,
};

void destroy(wl_resource* resource) {
  core::server::get().xdg_surface_map.erase(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id, wl_resource* wl_surface) {
  auto surface = core::server::get().surface_map.find(wl_surface);
  if (surface == core::server::get().surface_map.end()) {
    wl_client_post_implementation_error(
        client, "xdg_surface requested for unknown wl_surface");
    return;
  }
  wl_resource* resource =
      wl_resource_create(client, &xdg_surface_interface, 7, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  core::server::get().xdg_surface_map.emplace(resource,
      std::make_shared<XdgSurface>(resource, std::weak_ptr(surface->second)));
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
}
}  // namespace yabe::proto::xdg_shell::xdg_surface
