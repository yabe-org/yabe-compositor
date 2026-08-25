#include "popup.hpp"

#include <memory>

#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
PopupPointerGrab PopupPointerGrab::instance;

PopupPointerGrab* PopupPointerGrab::start(
    proto::xdg::shell::xdg_popup::XdgPopup* popup) {
  const auto popup_surface    = popup->popup_surface().lock();
  const auto popup_wl_surface = popup_surface->wl_surface().lock();
  server::get().seat.keyboard.set_focused_surface(popup_wl_surface);
  instance.popup_ = popup;
  return &instance;
}

void PopupPointerGrab::cleanup() {
}

void PopupPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(-dy, -dx);
  server::get().seat.pointer.focus_pointed_surface();
}

void PopupPointerGrab::button(uint32_t button, bool is_pressed) {
  const auto pointer_focused_scene_node =
      server::get().seat.pointer.get_focused_scene_node().lock();
  if (!pointer_focused_scene_node) {
    return;
  }
  // do not call seat.keyboard.set_focused_surface!

  pointer_focused_scene_node->button(button, is_pressed);
  pointer_focused_scene_node->frame();
}

bool PopupPointerGrab::is_active(
    proto::xdg::shell::xdg_popup::XdgPopup* popup) {
  return server::get().seat.pointer.is_active_grab(&instance) &&
         instance.popup_ == popup;
}
}  // namespace yabe::core::server_seat::pointer_grab
