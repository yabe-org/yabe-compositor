#pragma once

#include <functional>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "wayland-server.h"

namespace yabe::core::data_device {
class DataSource {
 public:
  DISABLE_MOVE_AND_COPY(DataSource);
  explicit DataSource(wl_resource* resource);
  ~DataSource() = default;

  void add_mime_type(const char* mime);
  void for_each_mime(const std::function<void(const std::string&)>& callback);

  [[nodiscard]] wl_resource* resource() const {
    return resource_;
  }

 private:
  std::vector<std::string> mimes_;
  wl_resource*             resource_;
};
}  // namespace yabe::core::data_device
