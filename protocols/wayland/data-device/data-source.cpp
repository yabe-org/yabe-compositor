#include "data-source.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

namespace yabe::proto::wayland::data_device_manager::data_source {
namespace {
void offer(wl_client* /*client*/, wl_resource* /*resource*/,
    const char* /*mime_type*/) {
  // TODO
}
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_actions(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*dnd_actions*/) {
  // TODO
}
constexpr struct wl_data_source_interface kImpl = {
    .offer       = offer,
    .destroy     = destroy,
    .set_actions = set_actions,
};

}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_source_interface, wl_data_source_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_source
