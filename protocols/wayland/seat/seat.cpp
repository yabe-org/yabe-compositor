#include "seat.hpp"

#include <wayland-server.h>

#include "core/server.hpp"
#include "keyboard.hpp"
#include "pointer.hpp"
#include "wayland-server-core.h"

namespace yabe::proto::wayland::seat {
namespace {
constexpr struct wl_seat_interface kImpl = {
    .get_pointer =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          pointer::create(client, wl_resource_get_version(resource), id);
        },
    .get_keyboard =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          keyboard::create(client, wl_resource_get_version(resource), id);
        },
    .get_touch =
        [](wl_client* client, wl_resource* /*resource*/, uint32_t /*id*/) {
          wl_client_post_implementation_error(
              client, "touch device is not implemented");
        },
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  auto* client = wl_resource_get_client(resource);
  core::server::get().seat.remove_client_seat(client);
}
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* client_seat = core::server::get().seat.create_client_seat(client);
  wl_resource_set_implementation(
      resource, &kImpl, client_seat, handle_destruction);
  wl_seat_send_capabilities(
      resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
}
}  // namespace yabe::proto::wayland::seat
