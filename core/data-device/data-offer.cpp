#include "data-offer.hpp"

#include <unistd.h>
#include <wayland-server-protocol.h>

namespace yabe::core::data_device {
DataOffer::DataOffer(wl_resource* resource, std::shared_ptr<DataSource>& source)
    : data_source_(source), resource_(resource) {
}

void DataOffer::accept(const char* mime) {
  const auto source = data_source_.lock();
  if (!source) {
    return;
  }
  source->send_target(mime);
}
void DataOffer::receive(const char* mime, int32_t fd) {
  const auto source = data_source_.lock();
  if (source) {
    source->send_send(mime, fd);
  }
  close(fd);
}
void DataOffer::finish_dnd() {
  const auto source = data_source_.lock();
  if (!source) {
    return;
  }
  source->send_dnd_finished();
}
void DataOffer::set_actions(
    uint32_t dnd_actions, wl_data_device_manager_dnd_action preferred_action) {
  this->dnd_actions_      = dnd_actions;
  this->preferred_action_ = preferred_action;
}

void DataOffer::send_offer(const std::string& mime) {
  wl_data_offer_send_offer(this->resource_, mime.c_str());
}
void DataOffer::send_source_actions(uint32_t actions) {
  if (wl_resource_get_version(this->resource_) <
      WL_DATA_OFFER_SOURCE_ACTIONS_SINCE_VERSION) {
    return;
  }
  wl_data_offer_send_source_actions(this->resource_, actions);
}
void DataOffer::send_action(uint32_t action) {
  if (wl_resource_get_version(this->resource_) <
      WL_DATA_OFFER_ACTION_SINCE_VERSION) {
    return;
  }
  wl_data_offer_send_action(this->resource_, action);
}
}  // namespace yabe::core::data_device
