#include "data-offer.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

#include <memory>

#include "core/data-device/data-offer.hpp"
#include "core/server.hpp"
#include "wayland-server-protocol.h"

namespace yabe::proto::wayland::data_device_manager::data_offer {
namespace {
constexpr struct wl_data_offer_interface kImpl = {
    .accept =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t /*serial*/,
            const char* mime_type) {
          core::server::get().data_offer_map.at(resource)->accept(mime_type);
        },
    .receive =
        [](wl_client* /*client*/, wl_resource* resource, const char* mime_type,
            int32_t fd) {
          core::server::get().data_offer_map.at(resource)->receive(
              mime_type, fd);
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .finish =
        [](wl_client* /*client*/, wl_resource* resource) {
          core::server::get().data_offer_map.at(resource)->finish_dnd();
        },
    .set_actions =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t dnd_actions,
            uint32_t preferred_action) {
          if (!wl_data_device_manager_dnd_action_is_valid(
                  preferred_action, wl_resource_get_version(resource))) {
            wl_resource_post_error(
                resource, WL_DATA_OFFER_ERROR_INVALID_ACTION, "");
            return;
          }
          core::server::get().data_offer_map.at(resource)->set_actions(
              dnd_actions,
              static_cast<wl_data_device_manager_dnd_action>(preferred_action));
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().data_offer_map.erase(resource);
}
}  // namespace

std::shared_ptr<core::data_device::DataOffer> create(
    wl_client* client, std::shared_ptr<core::data_device::DataSource> source) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_offer_interface, wl_data_offer_interface.version, 0);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return {};
  }
  const auto self =
      std::make_shared<core::data_device::DataOffer>(resource, source);
  core::server::get().data_offer_map.emplace(resource, self);
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  return self;
}
}  // namespace yabe::proto::wayland::data_device_manager::data_offer
