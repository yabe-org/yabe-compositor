#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "core/data-device/data-source.hpp"

namespace yabe::core::data_device {
class DataOffer {
 public:
  DISABLE_MOVE_AND_COPY(DataOffer);
  DataOffer(wl_resource* resource, std::shared_ptr<DataSource>& source);
  ~DataOffer() = default;

  void accept(const char* mime);
  void receive(const char* mime, int32_t fd);
  void finish_dnd();
  void set_actions(
      uint32_t dnd_action, wl_data_device_manager_dnd_action preferred_action);

  void send_offer(const std::string& mime);
  void send_source_actions(uint32_t actions);
  void send_action(uint32_t action);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] uint32_t dnd_actions() const {
    return this->dnd_actions_;
  }
  [[nodiscard]] wl_data_device_manager_dnd_action preferred_action() const {
    return this->preferred_action_;
  }
  [[nodiscard]] bool did_accept_mime() const {
    return this->did_accept_mime_;
  }

 private:
  bool     did_accept_mime_ = false;
  uint32_t dnd_actions_     = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  wl_data_device_manager_dnd_action preferred_action_ =
      WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  std::vector<std::string>  mimes_;
  std::weak_ptr<DataSource> data_source_;
  wl_resource*              resource_;
};
}  // namespace yabe::core::data_device
