#pragma once

#include <wayland-server-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/data-device/data-offer.hpp"
#include "core/data-device/data-source.hpp"
#include "core/surface.hpp"
#include "pointer-grab.hpp"

namespace yabe::core::server_seat::pointer_grab {
class DragAndDropPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(DragAndDropPointerGrab);

  static DragAndDropPointerGrab* start(
      std::weak_ptr<data_device::DataSource>&& data_source);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  explicit DragAndDropPointerGrab() = default;
  ~DragAndDropPointerGrab()         = default;

  void send_cancel_if_needed();

  void enter_pointed_surface();
  bool set_entered_surface(std::weak_ptr<Surface> surface);

  void                              renew_action();
  wl_data_device_manager_dnd_action select_action();

  wl_data_device_manager_dnd_action selected_action_ =
      WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  wl_data_device_manager_dnd_action prev_action_ =
      WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;

  std::weak_ptr<data_device::DataSource> data_source_;
  std::weak_ptr<data_device::DataOffer>  current_data_offer_;
  std::weak_ptr<Surface>                 entered_surface_;
};
}  // namespace yabe::core::server_seat::pointer_grab
