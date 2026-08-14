#include "data-device.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

#include <memory>

#include "core/data-device/data-device.hpp"
#include "core/data-device/data-source.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wayland::data_device_manager::data_device {
namespace {
constexpr struct wl_data_device_interface kImpl = {
    .start_drag =
        [](wl_client* /*client*/, wl_resource* resource, wl_resource* source,
            wl_resource* /*origin*/, wl_resource* icon, uint32_t /*serial*/) {
          std::shared_ptr<core::Surface> icon_surface;
          if (icon) {
            icon_surface = core::server::get().surface_map.at(icon);
          }
          std::shared_ptr<core::data_device::DataSource> data_source;
          if (source) {
            data_source = core::server::get().data_source_map.at(source);
          }
          core::server::get().data_device_map.at(resource)->start_drag(
              data_source, icon_surface);
        },
    .set_selection =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* data_source_resource, uint32_t /*serial*/) {
          if (data_source_resource == nullptr) {
            core::server::get().data_device_map.at(resource)->set_selection(
                nullptr);
            return;
          }
          auto source =
              core::server::get().data_source_map.at(data_source_resource);
          core::server::get().data_device_map.at(resource)->set_selection(
              source);
        },
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().data_device_map.erase(resource);

  auto*      client      = wl_resource_get_client(resource);
  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_data_device.destroy");
    return;
  }
  client_seat.value()->data_device.remove_data_device(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &wl_data_device_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  const auto self = std::make_shared<core::data_device::DataDevice>(resource);
  core::server::get().data_device_map.emplace(resource, self);
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);

  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    wl_client_post_no_memory(client);
    return;
  }
  client_seat.value()->data_device.add_data_device(resource);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_device
