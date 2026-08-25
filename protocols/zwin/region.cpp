#include "region.hpp"

#include <zwin-protocol.h>

#include <memory>

#include "common/convert-wl-array.hpp"
#include "core/server.hpp"
#include "wayland-server-core.h"

namespace yabe::proto::zwin::compositor {
void Region::add_cuboid(
    glm::vec3 half_size, glm::vec3 center, glm::quat rotation) {
  cuboids_.push_back({
      .half_size = half_size,
      .center    = center,
      .rotation  = rotation,
  });
}

namespace {
constexpr struct zwn_region_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .add_cuboid =
        [](wl_client* /*client*/, wl_resource* resource, wl_array* half_size,
            wl_array* center, wl_array* rotation) {
          const auto self = core::server::get().resources.get<Region>(resource);
          if (!self) {
            LOG_ERR("region not found on zwn_region.add_cuboid");
            return;
          }
          glm::vec3 half_size_vec;
          glm::vec3 center_vec;
          glm::quat rotation_quat;
          if (!common::from_wl_array(half_size, &half_size_vec) ||
              !common::from_wl_array(center, &center_vec) ||
              !common::from_wl_array(rotation, &rotation_quat)) {
            wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
                "invalid arguments for add_cuboid");
            return;
          }
          self->add_cuboid(half_size_vec, center_vec, rotation_quat);
        },
    .add_sphere =
        [](wl_client* client, wl_resource* /*resource*/, wl_array* /*center*/,
            wl_array* /*radius*/) {
          wl_client_post_implementation_error(
              client, "add_sphere is not yet implemented");
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_region(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_region_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<Region>();
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::compositor
