#include "pointer.hpp"

#include <wayland-server-core.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::seat::pointer {
namespace {
constexpr struct wl_pointer_interface kImpl = {
    .set_cursor =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            uint32_t /*serial*/, wl_resource* surface_resource,
            int32_t hotspot_x, int32_t hotspot_y) {
          core::server::get().seat.pointer.set_surface_as_cursor(
              surface_resource, hotspot_x, hotspot_y);
        },
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  auto*      client      = wl_resource_get_client(resource);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_pointer.destroy");
    return;
  }
  client_seat.value()->pointer.remove_pointer(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_pointer_interface, wl_pointer_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_pointer.create");
    wl_resource_destroy(resource);
    return;
  }
  client_seat.value()->pointer.add_pointer(resource);
}
}  // namespace yabe::proto::wayland::seat::pointer
