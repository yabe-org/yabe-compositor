#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "core/data-device/data-source.hpp"
#include "wayland-server.h"

namespace yabe::core::data_device {
class DataOffer {
 public:
  DISABLE_MOVE_AND_COPY(DataOffer);
  DataOffer(wl_resource* resource, std::shared_ptr<DataSource>& source);
  ~DataOffer() = default;

  void accept(const char* mime);
  void receive(const char* mime, int32_t fd);

  void send_offer(const std::string& mime);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }

 private:
  std::vector<std::string>  mimes_;
  std::weak_ptr<DataSource> data_source_;
  wl_resource*              resource_;
};
}  // namespace yabe::core::data_device
