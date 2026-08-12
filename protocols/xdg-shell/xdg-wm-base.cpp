#include "xdg-wm-base.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <xdg-shell-protocol.h>

#include "xdg-positioner.hpp"
#include "xdg-surface.hpp"

namespace yabe::proto::xdg_shell::xdg_wm_base {
namespace {
constexpr struct xdg_wm_base_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .create_positioner =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id) {
          xdg_positioner::create(client, id);
        },
    .get_xdg_surface =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id,
            wl_resource* surface_resource) {
          xdg_surface::create(client, id, surface_resource);
        },
    .pong =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            uint32_t /*serial*/) {
          // TODO
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_wm_base_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::xdg_shell::xdg_wm_base
