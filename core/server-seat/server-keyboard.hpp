#pragma once

#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon.h>

#include <memory>
#include <optional>
#include <vector>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/surface.hpp"
#include "wayland-server-protocol.h"

namespace yabe::core::server_seat {
enum class ModKeys : uint8_t {
  CTRL,
  ALT,  // Mod 1
  SHIFT,
  SUPER,  // Mod 4, aka Logo
  CAPS,
  NUM,  // Mod 2
  MOD3,
  MOD5
};

class ServerKeyboard {
 public:
  DISABLE_MOVE_AND_COPY(ServerKeyboard);
  explicit ServerKeyboard();
  ~ServerKeyboard();

  void set_create_keymap_func(
      const char* (*create_keymap)(const char* rules, const char* model,
          const char* layout, const char* variant, const char* options)) {
    this->create_keymap_ = create_keymap;
  }
  void init_keymap(const char* rules, const char* model, const char* layout,
      const char* variant, const char* options);
  void set_repeat_info(int32_t rate, int32_t delay);

  void handle_keyboard_key(uint32_t key, bool is_pressed);

  void send_init_events(wl_resource* resource);

  /// @return true if the focused surface is changed
  bool set_focused_surface(std::weak_ptr<Surface> surface);
  void set_current_focused_surface_as_exclusive();
  [[nodiscard]] const std::weak_ptr<Surface>& get_focused_surface() const {
    return this->focused_surface_;
  }

  /// @param keysym e.g. XKB_KEY_A, not XKB_KEY_a nor KEY_A
  [[nodiscard]] bool           is_key_pressed(xkb_keysym_t keysym) const;
  [[nodiscard]] bool           is_mod_pressed(ModKeys mod) const;
  [[nodiscard]] xkb_mod_mask_t try_convert_to_mod_mask(
      const char* mod_name) const;
  [[nodiscard]] xkb_mod_mask_t current_mod_mask() const {
    return this->mods_depressed_;
  }

 private:
  const char* (*create_keymap_)(const char* rules, const char* model,
      const char* layout, const char* variant, const char* options);

  /// @return a wl_array whose buffer is owned by std::vector,
  ///         so the caller must not release it
  [[nodiscard]] wl_array get_pressed_keys() const;
  void                   update_modifiers();

  xkb_context* xkb_context_ = nullptr;
  xkb_keymap*  xkb_keymap_  = nullptr;
  xkb_state*   xkb_state_   = nullptr;

  int32_t  keymap_fd_   = -1;
  uint32_t keymap_size_ = 0;

  uint32_t mods_depressed_ = 0;
  uint32_t mods_latched_   = 0;
  uint32_t mods_locked_    = 0;
  uint32_t mods_group_     = 0;

  int32_t repeat_rate_  = 33;
  int32_t repeat_delay_ = 500;

  std::vector<uint32_t> pressed_keys_;

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
                         get_focused_client_seat() const;
  std::weak_ptr<Surface> focused_surface_;
  std::weak_ptr<Surface> keyboard_exclusive_surface_;
};
}  // namespace yabe::core::server_seat
