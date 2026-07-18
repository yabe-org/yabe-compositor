#include "xdg-surface.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "protocols/xdg-shell/xdg-surface-role.hpp"
#include "xdg-toplevel.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
namespace {
void destroy(wl_client* /* client */, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_toplevel(wl_client* client, wl_resource* resource, uint32_t id) {
  auto* self =
      static_cast<XdgSurfaceRole*>(wl_resource_get_user_data(resource));
  auto* toplevel = xdg_toplevel::create(client, id, self->surface());
  if (!toplevel) {
    return;
  }
  auto* role =
      static_cast<XdgSurfaceRole*>(wl_resource_get_user_data(resource));
  role->set_xdg_role_object(toplevel);
}
void get_popup(wl_client* /*client*/, wl_resource* /* resource */,
    uint32_t /*id*/, wl_resource* /* parent */, wl_resource* /* positioner */) {
  // todo
}
void set_window_geometry(wl_client* /* client */, wl_resource* /* resource */,
    int32_t /* x */, int32_t /* y */, int32_t /* width */,
    int32_t /* height */) {
  // TODO
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
}  // namespace

void create(wl_client* client, uint32_t id, wl_resource* wl_surface) {
  auto surface = core::server::get().surfaces.get(wl_surface).lock();
  if (!surface) {
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
  auto  role_obj = std::make_unique<XdgSurfaceRole>(resource, surface);
  auto* p        = role_obj.get();
  surface->set_role(std::move(role_obj));
  wl_resource_set_implementation(resource, &kImpl, p, nullptr);
}
}  // namespace yabe::proto::xdg_shell::xdg_surface
