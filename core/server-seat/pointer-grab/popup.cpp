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
  if (instance.popup_stack_.empty() || instance.popup_stack_.back() != popup) {
    instance.popup_stack_.push_back(popup);
  }
  return &instance;
}

// NOLINTNEXTLINE(misc-no-recursion)
void PopupPointerGrab::remove(proto::xdg::shell::xdg_popup::XdgPopup* popup) {
  if (instance.popup_stack_.empty()) {
    return;
  }
  if (std::erase(instance.popup_stack_, popup) == 0) {
    return;
  }

  if (instance.popup_stack_.empty()) {
    if (server::get().seat.pointer.is_active_grab(&instance)) {
      server::get().seat.pointer.end_grab();
    }
    return;
  }

  auto*      next_popup    = instance.popup_stack_.back();
  const auto popup_surface = next_popup->popup_surface().lock();
  if (!popup_surface) {
    PopupPointerGrab::remove(next_popup);
    return;
  }
  const auto popup_wl_surface = popup_surface->wl_surface().lock();
  if (!popup_wl_surface) {
    PopupPointerGrab::remove(next_popup);
    return;
  }
  server::get().seat.keyboard.set_focused_surface(popup_wl_surface);
}

void PopupPointerGrab::cleanup() {
  this->popup_stack_.clear();
  // restore keyboard focus
  server::get().workspace.current_scene().focus_to_active_node();
}

void PopupPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(dy, dx);
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
}  // namespace yabe::core::server_seat::pointer_grab
