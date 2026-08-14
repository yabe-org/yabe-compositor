#pragma once

#include <functional>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "wayland-server-protocol.h"
#include "wayland-server.h"

namespace yabe::core::data_device {
class DataSource {
 public:
  DISABLE_MOVE_AND_COPY(DataSource);
  explicit DataSource(wl_resource* resource);
  ~DataSource() = default;

  void add_mime_type(const char* mime);
  void set_actions(uint32_t dnd_actions);

  void send_target(const char* mime);
  void send_send(const char* mime, int32_t fd);
  void send_dnd_drop_performed();
  void send_dnd_finished();
  void send_action(uint32_t dnd_action);

  void for_each_mime(const std::function<void(const std::string&)>& callback);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] uint32_t dnd_actions() const {
    return this->dnd_actions_;
  }

 private:
  std::vector<std::string> mimes_;
  uint32_t     dnd_actions_ = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  wl_resource* resource_;
};
}  // namespace yabe::core::data_device
