#include "compositor.hpp"

#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include "region.hpp"
#include "virtual-object.hpp"

namespace yabe::proto::zwin::compositor {
namespace {
constexpr struct zwn_compositor_interface kImpl = {
    .create_virtual_object =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_virtual_object(client, wl_resource_get_version(resource), id);
        },
    .create_region =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_region(client, wl_resource_get_version(resource), id);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_compositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::zwin::compositor
