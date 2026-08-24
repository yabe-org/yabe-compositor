#include "subcompositor.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "core/server.hpp"
#include "core/surface.hpp"
#include "subsurface.hpp"
#include "wayland-server-core.h"

namespace yabe::proto::wayland::subcompositor {
namespace {
constexpr struct wl_subcompositor_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_subsurface =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface, wl_resource* parent) {
          auto parent_surface =
              core::server::get().resources.get<core::Surface>(parent);
          if (!parent_surface) {
            wl_client_post_implementation_error(
                wl_resource_get_client(resource),
                "subsurface requested for unknown parent surface");
            return;
          }
          auto wl_surface =
              core::server::get().resources.get<core::Surface>(surface);
          subsurface::create(client, wl_resource_get_version(resource), id,
              wl_surface, parent_surface);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_subcompositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::subcompositor
