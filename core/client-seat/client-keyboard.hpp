#pragma once

#include <wayland-server.h>

#include <list>

#include "common/common.hpp"

namespace yabe::core::client_seat {
class ClientKeyboard {
 public:
  DISABLE_MOVE_AND_COPY(ClientKeyboard);
  explicit ClientKeyboard() = default;
  ~ClientKeyboard()         = default;

  void add_keyboard(wl_resource* resource);
  void remove_keyboard(wl_resource* resource);

  void send_keymap(uint32_t format, int32_t fd, uint32_t size);
  // FIXME: accept Surface, not wl_resource*,
  // to avoid confusion with other resources
  void send_enter(wl_resource* surface_resource, wl_array* keys);
  void send_leave(wl_resource* surface_resource);
  void send_key(uint32_t key, uint32_t state);
  void send_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group);
  void send_repeat_info(int32_t rate, int32_t delay);

 private:
  std::list<wl_resource*> keyboards_;
};
}  // namespace yabe::core::client_seat
