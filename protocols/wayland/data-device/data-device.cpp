#include "data-device.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

namespace yabe::proto::wayland::data_device_manager::data_device {
namespace {
void start_drag(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*source*/, wl_resource* /*origin*/, wl_resource* /*icon*/,
    uint32_t /*serial*/) {
  // TODO
}
void set_selection(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*data_source_resource*/, uint32_t /*serial*/) {
  // TODO
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_data_device_interface kImpl = {
    .start_drag    = start_drag,
    .set_selection = set_selection,
    .release       = release,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_interface, wl_data_device_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_device
