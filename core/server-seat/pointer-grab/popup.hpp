#pragma once

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "protocols/xdg-shell/xdg-popup.hpp"

namespace yabe::core::server_seat::pointer_grab {
class PopupPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(PopupPointerGrab);

  static PopupPointerGrab* start(proto::xdg_shell::xdg_popup::XdgPopup* popup);
  static bool is_active(proto::xdg_shell::xdg_popup::XdgPopup* popup);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  static PopupPointerGrab instance;
  explicit PopupPointerGrab() = default;
  ~PopupPointerGrab()         = default;

  // XdgPopup ends popup grab when it is destroyed
  proto::xdg_shell::xdg_popup::XdgPopup* popup_ = nullptr;
};
}  // namespace yabe::core::server_seat::pointer_grab
