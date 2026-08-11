#include "data-device.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

#include <memory>

#include "core/data-device/data-device.hpp"
#include "core/server.hpp"

namespace yabe::proto::wayland::data_device_manager::data_device {
namespace {
void start_drag(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*source*/, wl_resource* /*origin*/, wl_resource* /*icon*/,
    uint32_t /*serial*/) {
  // TODO
}
void set_selection(wl_client* /*client*/, wl_resource* resource,
    wl_resource* data_source_resource, uint32_t /*serial*/) {
  if (data_source_resource == nullptr) {
    core::server::get().data_device_map.at(resource)->set_selection(nullptr);
  } else {
    auto source = core::server::get().data_source_map.at(data_source_resource);
    core::server::get().data_device_map.at(resource)->set_selection(source);
  }
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_data_device_interface kImpl = {
    .start_drag    = start_drag,
    .set_selection = set_selection,
    .release       = release,
};
void destroy(wl_resource* resource) {
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

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_interface, wl_data_device_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  const auto self = std::make_shared<core::data_device::DataDevice>(resource);
  core::server::get().data_device_map.emplace(resource, self);
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);

  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    wl_client_post_no_memory(client);
    return;
  }
  client_seat.value()->data_device.add_data_device(resource);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_device
