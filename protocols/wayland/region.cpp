#include "region.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "core/region.hpp"
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
          const auto self =
              core::server::get().resources.get<core::Region>(resource);
          if (self) {
            self->add(x, y, width, height);
          }
        },
    .subtract =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          const auto self =
              core::server::get().resources.get<core::Region>(resource);
          if (self) {
            self->subtract(x, y, width, height);
          }
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
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
  const auto self = std::make_shared<core::Region>(resource);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::wayland::region
