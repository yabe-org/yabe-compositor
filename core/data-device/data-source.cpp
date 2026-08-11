#include "data-source.hpp"

namespace yabe::core::data_device {
DataSource::DataSource(wl_resource* resource) : resource_(resource) {
}

void DataSource::add_mime_type(const char* mime) {
  mimes_.emplace_back(mime);
}

void DataSource::for_each_mime(
    const std::function<void(const std::string&)>& callback) {
  for (const auto& mime : mimes_) {
    callback(mime);
  }
}
}  // namespace yabe::core::data_device
