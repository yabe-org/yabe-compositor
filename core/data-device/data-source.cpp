#include "data-source.hpp"

namespace yabe::core::data_device {
DataSource::DataSource(wl_resource* resource) : resource_(resource) {
}

void DataSource::add_mime_type(const char* mime) {
  mimes_.emplace_back(mime);
}
void DataSource::set_actions(uint32_t dnd_actions) {
  this->dnd_actions_ = dnd_actions;
}

void DataSource::send_target(const char* mime) {
  wl_data_source_send_target(this->resource_, mime);
}
void DataSource::send_send(const char* mime, int32_t fd) {
  wl_data_source_send_send(this->resource_, mime, fd);
}
void DataSource::send_dnd_drop_performed() {
  if (wl_resource_get_version(this->resource_) <
      WL_DATA_SOURCE_DND_DROP_PERFORMED_SINCE_VERSION) {
    return;
  }
  wl_data_source_send_dnd_drop_performed(this->resource_);
}
void DataSource::send_dnd_finished() {
  if (wl_resource_get_version(this->resource_) <
      WL_DATA_SOURCE_DND_FINISHED_SINCE_VERSION) {
    return;
  }
  wl_data_source_send_dnd_finished(this->resource_);
}
void DataSource::send_action(uint32_t dnd_action) {
  if (wl_resource_get_version(this->resource_) <
      WL_DATA_SOURCE_ACTION_SINCE_VERSION) {
    return;
  }
  wl_data_source_send_action(this->resource_, dnd_action);
}

void DataSource::for_each_mime(
    const std::function<void(const std::string&)>& callback) {
  for (const auto& mime : mimes_) {
    callback(mime);
  }
}
}  // namespace yabe::core::data_device
