#include "confined-pointer.hpp"

#include <pointer-constraints-protocol.h>
#include <wayland-server.h>

namespace yabe::proto::wayland::pointer_constraints::confined {
namespace {
constexpr struct zwp_confined_pointer_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_region =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*region*/) {
          // TODO
        },
};
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<core::Surface>& /*surface*/) {
  wl_resource* resource = wl_resource_create(
      client, &zwp_confined_pointer_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::pointer_constraints::confined
