#pragma once

#include <wayland-server.h>

#include <list>
#include <memory>

#include "common/common.hpp"
#include "core/keyboard.hpp"
#include "core/surface.hpp"

namespace yabe::core::client_seat {
class ClientKeyboard {
 public:
  DISABLE_MOVE_AND_COPY(ClientKeyboard);
  explicit ClientKeyboard() = default;
  ~ClientKeyboard()         = default;

  void add_keyboard(wl_resource* resource);
  void remove_keyboard(wl_resource* resource);

  void set_current_keyboard(const std::shared_ptr<Keyboard>& keyboard);
  [[nodiscard]] bool is_current_keyboard(
      const std::shared_ptr<Keyboard>& keyboard) const {
    return this->current_keyboard_.lock() == keyboard;
  }

  void send_keymap(uint32_t format, int32_t fd, uint32_t size);
  // FIXME: accept Surface, not wl_resource*,
  // to avoid confusion with other resources
  void send_enter(const std::shared_ptr<Surface>& surface, wl_array* keys);
  void send_leave(const std::shared_ptr<Surface>& surface);
  void send_key(uint32_t time, uint32_t key, uint32_t state);
  void send_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group);
  void send_repeat_info(int32_t rate, int32_t delay);

 private:
  std::list<wl_resource*> keyboards_;
  std::weak_ptr<Keyboard> current_keyboard_;
};
}  // namespace yabe::core::client_seat
