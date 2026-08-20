#pragma once

#include <xkbcommon/xkbcommon.h>

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/fields/dispatchers.hpp"

namespace yabe::core::setting::fields {
class Keybinds {
 public:
  DISABLE_MOVE_AND_COPY(Keybinds);
  explicit Keybinds() = default;
  ~Keybinds()         = default;

  void initialize(Engine* engine);
  void reset();

  /// @return true if the key event is handled by the keybinds
  bool handle_key(
      xkb_keysym_t key, bool is_pressed, xkb_mod_mask_t modifier_mask);

 private:
  /// @return false if it fails to parse key combination
  bool try_register_keybind(
      const std::vector<std::string>& key_strings, DispatcherId dispatcher_id);

  struct KeybindState {
    std::vector<std::pair<xkb_keysym_t, bool>> keys;
    xkb_mod_mask_t                             modifier_mask;
    DispatcherId                               dispatcher_id;
  };
  std::vector<KeybindState> keybinds_;
};
}  // namespace yabe::core::setting::fields
