#include "client-keyboard.hpp"

#include <wayland-server-core.h>

#include "common/common.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::core::client_seat {
void ClientKeyboard::add_keyboard(wl_resource* resource) {
  LOG_DBG("add keyboard#{}", wl_resource_get_id(resource));
  this->keyboards_.push_back(resource);
}
void ClientKeyboard::remove_keyboard(wl_resource* resource) {
  this->keyboards_.remove(resource);
}

void ClientKeyboard::set_current_keyboard(
    const std::shared_ptr<Keyboard>& keyboard) {
  if (this->current_keyboard_.lock() == keyboard) {
    return;
  }
  this->current_keyboard_ = keyboard;
  this->send_keymap(keyboard->keymap_format(), keyboard->keymap_fd(),
      keyboard->keymap_size());
  this->send_repeat_info(keyboard->repeat_rate(), keyboard->repeat_delay());
  this->send_modifiers(keyboard->mods().depressed, keyboard->mods().latched,
      keyboard->mods().locked, keyboard->mods().group);
}

void ClientKeyboard::send_keymap(uint32_t format, int32_t fd, uint32_t size) {
  for (auto* resource : this->keyboards_) {
    wl_keyboard_send_keymap(resource, format, fd, size);
  }
}

void ClientKeyboard::send_enter(
    const std::shared_ptr<Surface>& surface, wl_array* keys) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->keyboards_) {
    wl_keyboard_send_enter(resource, serial, surface->resource(), keys);
  }
}

void ClientKeyboard::send_leave(const std::shared_ptr<Surface>& surface) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->keyboards_) {
    wl_keyboard_send_leave(resource, serial, surface->resource());
  }
}

void ClientKeyboard::send_key(uint32_t time, uint32_t key, uint32_t state) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->keyboards_) {
    wl_keyboard_send_key(resource, serial, time, key, state);
  }
}

void ClientKeyboard::send_modifiers(uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->keyboards_) {
    wl_keyboard_send_modifiers(
        resource, serial, mods_depressed, mods_latched, mods_locked, group);
  }
}

void ClientKeyboard::send_repeat_info(int32_t rate, int32_t delay) {
  for (auto* resource : this->keyboards_) {
    if (wl_resource_get_version(resource) >=
        WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
      wl_keyboard_send_repeat_info(resource, rate, delay);
    }
  }
}
}  // namespace yabe::core::client_seat
