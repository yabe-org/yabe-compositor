#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include <memory>
#include <optional>
#include <vector>

#include "common/common.hpp"

namespace yabe::core {
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

struct ModMasks {
  uint32_t depressed = 0;
  uint32_t latched   = 0;
  uint32_t locked    = 0;
  uint32_t group     = 0;

  auto operator<=>(const ModMasks&) const = default;
};

class Keyboard : public std::enable_shared_from_this<Keyboard> {
 public:
  DISABLE_MOVE_AND_COPY(Keyboard);
  Keyboard();
  ~Keyboard();

  void set_owner(wl_client* client);
  void set_keymap(uint32_t format, int original_fd, size_t size);
  void set_repeat_info(int32_t rate, int32_t delay);

  /// @return true if mods are changed
  bool update_xkb_state(uint32_t key, bool is_pressed);
  void handle_key(uint32_t key, bool is_pressed);
  void handle_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group);

  [[nodiscard]] bool is_valid() const {
    return this->xkb_state_ != nullptr;
  }

  /// @param keysym e.g. XKB_KEY_A, not XKB_KEY_a nor KEY_A
  [[nodiscard]] bool           is_key_pressed(xkb_keysym_t keysym) const;
  [[nodiscard]] bool           is_mod_pressed(ModKeys mod) const;
  [[nodiscard]] xkb_mod_mask_t try_convert_to_mod_mask(
      const char* mod_name) const;
  [[nodiscard]] xkb_mod_mask_t current_mod_mask() const {
    return this->mods_.depressed;
  }
  [[nodiscard]] xkb_keysym_t get_one_sym(xkb_keycode_t xkb_keycode) const {
    return xkb_state_key_get_one_sym(this->xkb_state_, xkb_keycode);
  }

  [[nodiscard]] std::optional<wl_client*> owner() const {
    return this->owner_;
  }
  [[nodiscard]] uint32_t keymap_format() const {
    return this->keymap_format_;
  }
  [[nodiscard]] int keymap_fd() const {
    return this->keymap_fd_;
  }
  [[nodiscard]] uint32_t keymap_size() const {
    return this->keymap_size_;
  }
  [[nodiscard]] const ModMasks& mods() const {
    return this->mods_;
  }
  [[nodiscard]] int32_t repeat_rate() const {
    return this->repeat_rate_;
  }
  [[nodiscard]] int32_t repeat_delay() const {
    return this->repeat_delay_;
  }
  /// @return a wl_array whose buffer is owned by std::vector,
  ///         so the caller must not release it
  [[nodiscard]] wl_array get_pressed_keys() const;

 private:
  std::optional<wl_client*> owner_;

  xkb_context* xkb_context_ = nullptr;  // can be null
  xkb_keymap*  xkb_keymap_  = nullptr;
  xkb_state*   xkb_state_   = nullptr;

  uint32_t keymap_format_ = XKB_KEYMAP_FORMAT_TEXT_V1;
  int32_t  keymap_fd_     = -1;
  uint32_t keymap_size_   = 0;
  ModMasks mods_;
  int32_t  repeat_rate_  = 33;
  int32_t  repeat_delay_ = 500;

  std::vector<uint32_t> pressed_keys_;
};
}  // namespace yabe::core
