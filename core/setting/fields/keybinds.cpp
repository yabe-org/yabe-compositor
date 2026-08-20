#include "keybinds.hpp"

#include <strings.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon.h>

#include "common/common.hpp"
#include "core/server.hpp"
#include "core/setting/schema/function-entry.hpp"
#include "dispatchers.hpp"

namespace yabe::core::setting::fields {
void Keybinds::initialize(Engine* engine) {
  engine->create_field("keybind",
      schema::FunctionEntry<void(std::vector<std::string>, int)>::create(
          [this](std::vector<std::string> keys, int dispatcher_id) {
            if (!server::get().setting.dispatchers.is_valid(dispatcher_id)) {
              server::get().setting.add_error_message(
                  "keybind: 'dispatcher' is invalid");
              return;
            }
            if (!this->try_register_keybind(keys, dispatcher_id)) {
              server::get().setting.add_error_message(
                  std::format("keybind: 'keys' might be invalid ({})", keys));
            }
            return;
          }));
}

void Keybinds::reset() {
  this->keybinds_.clear();
}

bool Keybinds::handle_key(
    xkb_keysym_t keysym, bool is_pressed, xkb_mod_mask_t modifier_mask) {
  // keybind has lower keysyms because of XKB_KEYSYM_CASE_INSENSITIVE
  keysym       = xkb_keysym_to_lower(keysym);
  // FIXME: prevent sending release event by properly setting `handled``
  bool handled = false;
  for (auto& state : this->keybinds_) {
    bool all_keys_pressed = true;
    // update state and check if all keys are pressed
    for (auto& key : state.keys) {
      if (key.first == keysym) {
        key.second = is_pressed;
      }
      all_keys_pressed &= key.second;
    }

    if (!all_keys_pressed) {
      continue;
    }
    if ((state.modifier_mask & modifier_mask) != state.modifier_mask) {
      continue;
    }

    server::get().setting.dispatchers.dispatch(state.dispatcher_id);
    handled = true;
  }
  return handled;
}

bool Keybinds::try_register_keybind(
    const std::vector<std::string>& key_strings, DispatcherId dispatcher_id) {
  KeybindState keybind{
      .keys          = {},
      .modifier_mask = 0,
      .dispatcher_id = dispatcher_id,
  };
  for (const auto& key : key_strings) {
    const auto mod_mask =
        server::get().seat.keyboard.try_convert_to_mod_mask(key.c_str());
    if (mod_mask != 0) {
      keybind.modifier_mask |= mod_mask;
      continue;
    }

    const auto keysym =
        xkb_keysym_from_name(key.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);
    if (keysym == XKB_KEY_NoSymbol) {
      LOG_WARN("unknown key: {}", key);
      return false;
    }
    keybind.keys.emplace_back(keysym, false);
  }

  this->keybinds_.emplace_back(std::move(keybind));
  return true;
}
}  // namespace yabe::core::setting::fields
