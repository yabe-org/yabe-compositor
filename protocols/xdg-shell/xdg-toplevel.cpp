#include "xdg-toplevel.hpp"

#include <wayland-server-protocol.h>

#include <memory>

#include "core/server-seat/pointer-grab/move.hpp"
#include "core/server-seat/pointer-grab/resize.hpp"
#include "core/server.hpp"
#include "protocols/xdg-shell/xdg-surface.hpp"
#include "wayland-server-core.h"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_toplevel {
XdgToplevel::XdgToplevel(wl_resource*        resource,
    std::weak_ptr<xdg_surface::XdgSurface>&& surface,
    std::weak_ptr<core::scene::Toplevel>&&   toplevel)
    : resource_(resource)
    , xdg_surface_(std::move(surface))
    , toplevel_(std::move(toplevel)) {
}

void XdgToplevel::commit() {
  if (this->is_initial_commit_sent_) {
    return;
  }
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  this->is_initial_commit_sent_ = true;
  xdg_surface->configure();
}

void XdgToplevel::set_activated(bool is_activated) {
  this->is_activated_ = is_activated;
  this->send_configure(0, 0);
}

void XdgToplevel::start_move() {
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(
          std::weak_ptr(this->toplevel_)));
}
void XdgToplevel::start_resize(uint32_t edges) {
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::ResizePointerGrab::start(
          this->shared_from_this(), edges));
}

void XdgToplevel::request_resize(
    core::unit::Pixel width, core::unit::Pixel height) {
  this->send_configure(*width, *height);
}

void XdgToplevel::send_configure(int32_t width, int32_t height) {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  wl_array states;
  wl_array_init(&states);
  if (this->is_activated_) {
    auto* state =
        static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t)));
    if (state) {
      *state = XDG_TOPLEVEL_STATE_ACTIVATED;
    }
  }
  xdg_toplevel_send_configure(this->resource_, width, height, &states);
  wl_array_release(&states);
  xdg_surface->configure();
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
  core::server::get().xdg_toplevel_map.at(resource)->start_move();
}
void resize(wl_client* /*client*/, wl_resource* resource, wl_resource* /*seat*/,
    uint32_t /*serial*/, uint32_t               edges) {
  core::server::get().xdg_toplevel_map.at(resource)->start_resize(edges);
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

void destroy(wl_resource* resource) {
  if (auto toplevel = core::server::get().xdg_toplevel_map.at(resource)) {
    if (auto scene_node = toplevel->scene_node().lock()) {
      core::server::get().scene.erase(scene_node.get());
    }
  }
  core::server::get().xdg_toplevel_map.erase(resource);
}
}  // namespace

wl_resource* create(wl_client* client, uint32_t id,
    std::weak_ptr<xdg_surface::XdgSurface> xdg_surface) {
  const auto locked_xdg_surface = xdg_surface.lock();
  if (!locked_xdg_surface) {
    return nullptr;
  }
  const auto surface = locked_xdg_surface->wl_surface().lock();
  if (!surface) {
    return nullptr;
  }
  wl_resource* resource = wl_resource_create(
      client, &xdg_toplevel_interface, xdg_toplevel_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto toplevel = std::make_shared<core::scene::Toplevel>(surface);
  auto self     = std::make_shared<XdgToplevel>(
      resource, std::move(xdg_surface), std::weak_ptr(toplevel));
  core::server::get().scene.insert(std::move(toplevel));
  core::server::get().xdg_toplevel_map.emplace(resource, self);
  surface->set_role(self);
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  return resource;
}
}  // namespace yabe::proto::xdg_shell::xdg_toplevel
