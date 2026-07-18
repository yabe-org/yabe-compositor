#include "xdg-surface-role.hpp"

#include <wayland-util.h>

#include "core/server.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_surface {
XdgSurfaceRole::XdgSurfaceRole(
    wl_resource* xdg_surface, std::weak_ptr<core::Surface>&& wl_surface)
    : wl_surface_(std::move(wl_surface)), xdg_surface_(xdg_surface) {
}

void XdgSurfaceRole::commit() {
  if (!this->is_first_commit_) {
    return;
  }
  this->is_first_commit_ = false;
  xdg_surface_send_configure(
      this->xdg_surface_, core::server::get().next_serial());
}

void XdgSurfaceRole::set_activated(bool is_activated) {
  if (!this->xdg_role_object_) {
    return;
  }
  LOG_DBG("activated={}", is_activated);
  wl_array array;
  wl_array_init(&array);
  if (is_activated) {
    auto* e = static_cast<uint32_t*>(wl_array_add(&array, sizeof(uint32_t)));
    *e      = XDG_TOPLEVEL_STATE_ACTIVATED;
  }
  xdg_toplevel_send_configure(this->xdg_role_object_, 0, 0, &array);
  wl_array_release(&array);
  xdg_surface_send_configure(
      this->xdg_surface_, core::server::get().next_serial());
}

void XdgSurfaceRole::set_xdg_role_object(wl_resource* role_object) {
  this->xdg_role_object_ = role_object;
}
}  // namespace yabe::proto::xdg_shell::xdg_surface
