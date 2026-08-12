#include "data-device-manager.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "data-device.hpp"
#include "data-source.hpp"

namespace yabe::proto::wayland::data_device_manager {
namespace {
constexpr struct wl_data_device_manager_interface kImpl = {
    .create_data_source =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id) {
          data_source::create(client, id);
        },
    .get_data_device =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id,
            wl_resource* /*seat*/) {
          data_device::create(client, id);
        },
    .release =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_manager_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::data_device_manager
