#include "layer-shell.hpp"

#include <wayland-server.h>
#include <wlr-layer-shell-protocol.h>

#include "core/server.hpp"
#include "layer-surface.hpp"

namespace yabe::proto::wlr::layer_shell {
namespace {
constexpr struct zwlr_layer_shell_v1_interface kImpl = {
    .get_layer_surface =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface_resource, wl_resource* /*output_resource*/,
            uint32_t layer, const char* name_space) {
          auto wl_surface =
              core::server::get().surface_map.at(surface_resource);
          layer_surface::create(client, wl_resource_get_version(resource), id,
              wl_surface, static_cast<zwlr_layer_shell_v1_layer>(layer),
              name_space);
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwlr_layer_shell_v1_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wlr::layer_shell
