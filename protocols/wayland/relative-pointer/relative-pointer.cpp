#include "relative-pointer.hpp"

#include <relative-pointer-protocol.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::relative_pointer {
namespace {
constexpr struct zwp_relative_pointer_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  auto*      client      = wl_resource_get_client(resource);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_relative_pointer.destroy");
    return;
  }
  client_seat.value()->relative_pointer.remove_relative_pointer(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwp_relative_pointer_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_relative_pointer.create");
    wl_resource_destroy(resource);
    return;
  }
  client_seat.value()->relative_pointer.add_relative_pointer(resource);
}
}  // namespace yabe::proto::wayland::relative_pointer
