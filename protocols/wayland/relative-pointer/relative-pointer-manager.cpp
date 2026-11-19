#include "relative-pointer-manager.hpp"

#include <relative-pointer-protocol.h>
#include <wayland-server-core.h>

#include "protocols/wayland/relative-pointer/relative-pointer.hpp"

namespace yabe::proto::wayland::relative_pointer {
namespace {
constexpr struct zwp_relative_pointer_manager_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_relative_pointer =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* /*pointer*/) {
          create(client, wl_resource_get_version(resource), id);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwp_relative_pointer_manager_v1_interface,
          static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::relative_pointer
