#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"
#include "core/keyboard.hpp"

namespace yabe::proto::wlr::input_method {
class KeyboardGrab : public std::enable_shared_from_this<KeyboardGrab> {
 public:
  DISABLE_MOVE_AND_COPY(KeyboardGrab);
  explicit KeyboardGrab(wl_resource* resource);
  ~KeyboardGrab() = default;

  void set_current_keyboard(const std::shared_ptr<core::Keyboard>& keyboard);
  [[nodiscard]] bool is_current_keyboard(
      const std::shared_ptr<core::Keyboard>& keyboard) {
    return this->current_keyboard_.lock() == keyboard;
  }

  void send_keymap(uint32_t format, int32_t fd, uint32_t size);
  void send_repeat_info(int32_t rate, int32_t delay);
  void send_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
  void send_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group);

  [[nodiscard]] wl_client* owner() const {
    return wl_resource_get_client(this->resource_);
  }

 private:
  wl_resource*                  resource_;
  std::weak_ptr<core::Keyboard> current_keyboard_;
};

void create_keyboard_grab(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::wlr::input_method
