#include "ray.hpp"

#include <zwin-protocol.h>

#include "core/server.hpp"

namespace yabe::proto::zwin::seat {
namespace {
constexpr struct zwn_ray_interface kImpl = {
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
}  // namespace

void create_ray(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_ray_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on zwn_ray.create");
    wl_resource_destroy(resource);
    return;
  }
  client_seat.value()->ray.add_ray(resource);
}
}  // namespace yabe::proto::zwin::seat
