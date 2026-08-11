#include "client-data-device.hpp"

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
void ClientDataDevice::send_selection(
    const std::optional<std::shared_ptr<data_device::DataOffer>>& data_offer) {
  auto* data_offer_resource =
      data_offer ? data_offer.value()->resource() : nullptr;
  for (auto* resource : this->data_devices_) {
    wl_data_device_send_selection(resource, data_offer_resource);
  }
}
}  // namespace yabe::core::client_seat
