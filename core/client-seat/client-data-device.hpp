#pragma once

#include <wayland-server.h>

#include <list>
#include <memory>

#include "common/common.hpp"
#include "core/data-device/data-offer.hpp"

namespace yabe::core::client_seat {
class ClientDataDevice {
 public:
  DISABLE_MOVE_AND_COPY(ClientDataDevice);
  explicit ClientDataDevice() = default;
  ~ClientDataDevice()         = default;

  void add_data_device(wl_resource* resource);
  void remove_data_device(wl_resource* resource);

  void send_data_offer(
      const std::shared_ptr<data_device::DataOffer>& data_offer);
  void send_selection(
      const std::optional<std::shared_ptr<data_device::DataOffer>>& data_offer);

 private:
  std::list<wl_resource*> data_devices_;
};
}  // namespace yabe::core::client_seat
