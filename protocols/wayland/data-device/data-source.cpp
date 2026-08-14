#include "data-source.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::data_device_manager::data_source {
namespace {
constexpr struct wl_data_source_interface kImpl = {
    .offer =
        [](wl_client* /*client*/, wl_resource* resource,
            const char* mime_type) {
          core::server::get().data_source_map.at(resource)->add_mime_type(
              mime_type);
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_actions =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t dnd_actions) {
          core::server::get().data_source_map.at(resource)->set_actions(
              dnd_actions);
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().data_source_map.erase(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &wl_data_source_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  const auto self = std::make_shared<core::data_device::DataSource>(resource);
  core::server::get().data_source_map.emplace(resource, self);
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_source
