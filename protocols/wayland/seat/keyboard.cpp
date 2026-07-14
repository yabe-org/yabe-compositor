#include "keyboard.hpp"

#include <wayland-server-core.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::seat::keyboard {
namespace {
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_keyboard_interface kImpl = {
    .release = release,
};

void destroy(wl_resource* resource) {
  auto*      client      = wl_resource_get_client(resource);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_keyboard.destroy");
    return;
  }
  client_seat.value()->keyboard.remove_keyboard(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_keyboard_interface, wl_keyboard_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_keyboard.create");
    wl_resource_destroy(resource);
    return;
  }
  client_seat.value()->keyboard.add_keyboard(resource);
  core::server::get().seat.keyboard.send_init_events(resource);
}
}  // namespace yabe::proto::wayland::seat::keyboard
