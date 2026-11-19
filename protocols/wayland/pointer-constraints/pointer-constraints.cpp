#include "pointer-constraints.hpp"

#include <pointer-constraints-protocol.h>
#include <wayland-server-core.h>

#include "confined-pointer.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"
#include "locked-pointer.hpp"

namespace yabe::proto::wayland::pointer_constraints {
namespace {
constexpr struct zwp_pointer_constraints_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .lock_pointer =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface_resource, wl_resource* /*pointer*/,
            wl_resource* /*region*/, uint32_t lifetime) {
          const auto surface = core::server::get().resources.get<core::Surface>(
              surface_resource);
          if (surface) {
            locked::create(client, wl_resource_get_version(resource), id,
                surface,
                static_cast<zwp_pointer_constraints_v1_lifetime>(lifetime));
          }
        },
    .confine_pointer =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface_resource, wl_resource* /*pointer*/,
            wl_resource* /*region*/, uint32_t /*lifetime*/) {
          // TODO: treat region etc
          const auto surface = core::server::get().resources.get<core::Surface>(
              surface_resource);
          if (surface) {
            confined::create(
                client, wl_resource_get_version(resource), id, surface);
          }
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(client,
      &zwp_pointer_constraints_v1_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::pointer_constraints
