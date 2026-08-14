#include "client-data-device.hpp"

#include "common/time.hpp"
#include "core/server.hpp"
#include "wayland-server-protocol.h"

namespace yabe::core::client_seat {
void ClientDataDevice::add_data_device(wl_resource* resource) {
  LOG_DBG("add data_device#{}", wl_resource_get_id(resource));
  this->data_devices_.push_back(resource);
}
void ClientDataDevice::remove_data_device(wl_resource* resource) {
  this->data_devices_.remove(resource);
}

void ClientDataDevice::send_data_offer(
    const std::shared_ptr<data_device::DataOffer>& data_offer) {
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_data_offer(resource, data_offer->resource());
  }
}
void ClientDataDevice::send_enter(const std::shared_ptr<Surface>& surface,
    float x, float y,
    std::optional<std::shared_ptr<data_device::DataOffer>> data_offer) {
  const auto serial  = server::get().next_serial();
  const auto fixed_x = wl_fixed_from_double(x);
  const auto fixed_y = wl_fixed_from_double(y);
  auto*      data_offer_resource =
      data_offer ? data_offer.value()->resource() : nullptr;
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_enter(resource, serial, surface->resource(), fixed_x,
        fixed_y, data_offer_resource);
  }
}
void ClientDataDevice::send_leave() {
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_leave(resource);
  }
}
void ClientDataDevice::send_motion(float x, float y) {
  const auto now     = common::now_msec();
  const auto fixed_x = wl_fixed_from_double(x);
  const auto fixed_y = wl_fixed_from_double(y);
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_motion(resource, now, fixed_x, fixed_y);
  }
}
void ClientDataDevice::send_drop() {
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_drop(resource);
  }
}
void ClientDataDevice::send_selection(
    const std::optional<std::shared_ptr<data_device::DataOffer>>& data_offer) {
  auto* data_offer_resource =
      data_offer ? data_offer.value()->resource() : nullptr;
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_selection(resource, data_offer_resource);
  }
}
}  // namespace yabe::core::client_seat
