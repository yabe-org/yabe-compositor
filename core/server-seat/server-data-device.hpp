#pragma once

#include <memory>

#include "common/common.hpp"
#include "core/data-device/data-device.hpp"

namespace yabe::core::server_seat {
class ServerDataDevice {
 public:
  DISABLE_MOVE_AND_COPY(ServerDataDevice);
  explicit ServerDataDevice() = default;
  ~ServerDataDevice()         = default;

  void set_selection(std::shared_ptr<data_device::DataDevice> data_device);
  void send_selection(wl_client* client);

 private:
  std::weak_ptr<data_device::DataDevice> selected_;
};
}  // namespace yabe::core::server_seat
