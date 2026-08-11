#include "data-device.hpp"

#include "core/server.hpp"
#include "wayland-server-protocol.h"

namespace yabe::core::data_device {
DataDevice::DataDevice(wl_resource* resource) : resource_(resource) {
}

void DataDevice::set_selection(std::shared_ptr<DataSource> source) {  // NOLINT
  this->source_ = source;
  server::get().seat.data_device.set_selection(this->shared_from_this());
}
}  // namespace yabe::core::data_device
