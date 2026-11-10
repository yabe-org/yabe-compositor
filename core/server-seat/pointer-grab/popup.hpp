#pragma once

#include <vector>

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "protocols/xdg/shell/xdg-popup.hpp"

namespace yabe::core::server_seat::pointer_grab {
class PopupPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(PopupPointerGrab);

  static PopupPointerGrab* start(proto::xdg::shell::xdg_popup::XdgPopup* popup);
  static void remove(proto::xdg::shell::xdg_popup::XdgPopup* popup);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  static PopupPointerGrab instance;
  explicit PopupPointerGrab() = default;
  ~PopupPointerGrab()         = default;

  // XdgPopup must remove itself from this stack by calling remove()
  std::vector<proto::xdg::shell::xdg_popup::XdgPopup*> popup_stack_;
};
}  // namespace yabe::core::server_seat::pointer_grab
