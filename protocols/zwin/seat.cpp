#include "seat.hpp"

#include <zwin-protocol.h>

#include "core/server.hpp"
#include "protocols/zwin/ray.hpp"
#include "wayland-server-core.h"

namespace yabe::proto::zwin::seat {
namespace {
constexpr struct zwn_seat_interface kImpl = {
    .get_ray =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          create_ray(client, wl_resource_get_version(resource), id);
        },
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  core::server::get().seat.create_client_seat(client);
  zwn_seat_send_capabilities(resource,
      ZWN_SEAT_CAPABILITY_RAY_ORIGIN | ZWN_SEAT_CAPABILITY_RAY_DIRECTION);
}
}  // namespace yabe::proto::zwin::seat
