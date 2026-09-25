#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/keyboard.hpp"
#include "core/surface.hpp"
#include "protocols/wlr/input-method/keyboard-grab.hpp"

namespace yabe::core::server_seat {
class ServerKeyboard {
 public:
  DISABLE_MOVE_AND_COPY(ServerKeyboard);
  ServerKeyboard();
  ~ServerKeyboard() = default;

  void set_create_keymap_func(
      const char* (*create_keymap)(const char* rules, const char* model,
          const char* layout, const char* variant, const char* options)) {
    this->create_keymap_ = create_keymap;
  }
  void init_keymap(const char* rules, const char* model, const char* layout,
      const char* variant, const char* options);
  void initialize_wl_keyboard(wl_resource* wl_keyboard);

  void handle_external_keyboard_key(uint32_t key, bool is_pressed);
  void send_key(std::shared_ptr<Keyboard> keyboard, uint32_t time, uint32_t key,
      bool is_pressed);
  void send_modifiers(std::shared_ptr<Keyboard> keyboard,
      uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
      uint32_t group);
  void handle_keyboard_keymap_changed(std::shared_ptr<Keyboard> keyboard);
  void handle_keyboard_repeat_info_changed(std::shared_ptr<Keyboard> keyboard);

  void set_keyboard_grab(
      std::shared_ptr<proto::wlr::input_method::KeyboardGrab>& grab);
  void try_remove_keyboard_grab(
      std::shared_ptr<proto::wlr::input_method::KeyboardGrab> grab);

  void set_keyboard_grab(wl_resource* grab);
  void try_remove_keyboard_grab(wl_resource* grab);

  /// @return true if the focused surface is changed
  bool set_focused_surface(std::weak_ptr<Surface> surface);
  void set_current_focused_surface_as_exclusive();
  [[nodiscard]] const std::weak_ptr<Surface>& get_focused_surface() const {
    return this->focused_surface_;
  }

  [[nodiscard]] std::shared_ptr<Keyboard> current_keyboard() {
    auto keyboard = this->current_keyboard_.lock();
    return keyboard ? keyboard : this->external_keyboard_;
  }
  [[nodiscard]] const std::shared_ptr<Keyboard>& external_keyboard() {
    return this->external_keyboard_;
  }

 private:
  void send_initial_events(
      const std::shared_ptr<client_seat::ClientSeat>& client_seat);

  const char* (*create_keymap_)(const char* rules, const char* model,
      const char* layout, const char* variant, const char* options);

  /// keyboard whose events are sent via yabe_compositor_key()
  std::shared_ptr<Keyboard> external_keyboard_;
  std::weak_ptr<Keyboard>   current_keyboard_;
  void set_current_keyboard(std::shared_ptr<Keyboard>& keyboard);

  std::weak_ptr<proto::wlr::input_method::KeyboardGrab> keyboard_grab_;

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
                         get_focused_client_seat() const;
  std::weak_ptr<Surface> focused_surface_;
  std::weak_ptr<Surface> keyboard_exclusive_surface_;
};
}  // namespace yabe::core::server_seat
