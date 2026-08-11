#include "data-offer.hpp"

#include "wayland-server-protocol.h"

namespace yabe::core::data_device {
DataOffer::DataOffer(wl_resource* resource, std::shared_ptr<DataSource>& source)
    : data_source_(source), resource_(resource) {
}

void DataOffer::accept(const char* mime) {
  const auto source = data_source_.lock();
  if (!source) {
    return;
  }
  wl_data_source_send_target(source->resource(), mime);
}

void DataOffer::receive(const char* mime, int32_t fd) {
  const auto source = data_source_.lock();
  if (!source) {
    return;
  }
  wl_data_source_send_send(source->resource(), mime, fd);
}

void DataOffer::send_offer(const std::string& mime) {
  wl_data_offer_send_offer(this->resource_, mime.c_str());
}
}  // namespace yabe::core::data_device
