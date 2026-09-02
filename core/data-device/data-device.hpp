#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "core/data-device/data-source.hpp"
#include "core/surface.hpp"

namespace yabe::core::data_device {
class DataDevice : public std::enable_shared_from_this<DataDevice> {
 public:
  DISABLE_MOVE_AND_COPY(DataDevice);
  explicit DataDevice(wl_resource* resource);
  ~DataDevice() = default;

  void start_drag(std::optional<std::shared_ptr<DataSource>> source,
      std::optional<std::weak_ptr<Surface>>                  icon_surface);
  void set_selection(std::shared_ptr<DataSource> source);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::weak_ptr<DataSource> data_source() const {
    return this->source_;
  }

 private:
  std::weak_ptr<DataSource> source_;
  wl_resource*              resource_;
};
}  // namespace yabe::core::data_device
