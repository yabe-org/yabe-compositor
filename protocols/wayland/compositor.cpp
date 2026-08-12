#include "compositor.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cstdint>

#include "region.hpp"
#include "surface.hpp"

namespace yabe::proto::wayland::compositor {
namespace {
constexpr struct wl_compositor_interface kImpl = {
    .create_surface =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id) {
          surface::create(client, id);
        },
    .create_region =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t id) {
          region::create(client, id);
        },
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        }};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_compositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::compositor
