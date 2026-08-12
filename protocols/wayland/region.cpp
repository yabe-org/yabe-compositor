#include "region.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::region {
namespace {
constexpr struct wl_region_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .add =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          core::server::get().region_map.at(resource)->add(x, y, width, height);
        },
    .subtract =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          core::server::get().region_map.at(resource)->subtract(
              x, y, width, height);
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().region_map.erase(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_region_interface, wl_region_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  core::server::get().region_map.emplace(
      resource, std::make_unique<core::Region>(resource));
}
}  // namespace yabe::proto::wayland::region
