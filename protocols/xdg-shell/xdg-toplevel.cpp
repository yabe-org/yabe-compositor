#include "xdg-toplevel.hpp"

#include <wayland-server-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/move.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"
#include "wayland-server-core.h"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_toplevel {
XdgToplevel::XdgToplevel(std::weak_ptr<core::Surface>&& surface)
    : surface_(std::move(surface)) {
}

void XdgToplevel::move() {
  LOG_DBG("move");
  auto surface = this->surface_.lock();
  if (!surface) {
    return;
  }
  LOG_DBG(
      "move started for surface {}", wl_resource_get_id(surface->resource()));
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(surface));
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_parent(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*parent*/) {
  // TODO
}
void set_title(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*title*/) {
  // TODO
}
void set_app_id(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*app_id*/) {
  // TODO
}
void show_window_menu(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/, int32_t /*x*/, int32_t /*y*/) {
  // TODO
}
void move(wl_client* /*client*/, wl_resource* resource, wl_resource* /*seat*/,
    uint32_t /*serial*/) {
  auto* self = static_cast<XdgToplevel*>(wl_resource_get_user_data(resource));
  self->move();
}
void resize(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t /*edges*/) {
  // TODO
}
void set_max_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_min_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_maximized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void unset_maximized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_fullscreen(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*output*/) {
  // TODO
}
void unset_fullscreen(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_minimized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}

constexpr struct xdg_toplevel_interface kImpl = {
    .destroy          = destroy,
    .set_parent       = set_parent,
    .set_title        = set_title,
    .set_app_id       = set_app_id,
    .show_window_menu = show_window_menu,
    .move             = move,
    .resize           = resize,
    .set_max_size     = set_max_size,
    .set_min_size     = set_min_size,
    .set_maximized    = set_maximized,
    .unset_maximized  = unset_maximized,
    .set_fullscreen   = set_fullscreen,
    .unset_fullscreen = unset_fullscreen,
    .set_minimized    = set_minimized,
};

}  // namespace

wl_resource* create(
    wl_client* client, uint32_t id, std::weak_ptr<core::Surface>&& surface) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_toplevel_interface, xdg_toplevel_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto* self = new XdgToplevel(std::move(surface));
  wl_resource_set_implementation(
      resource, &kImpl, self, [](wl_resource* resource) {
        delete static_cast<XdgToplevel*>(wl_resource_get_user_data(resource));
      });
  return resource;
}
}  // namespace yabe::proto::xdg_shell::xdg_toplevel
