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
  using KeybindEntry =
      schema::FunctionEntry<void(std::vector<std::string>, DispatcherId)>;
  engine->create_field("keybind",
      KeybindEntry::create(
          {
              .description     = "Register a keybind",
              .parameter_names = {"keys", "dispatcher"},
  },
          [this](std::vector<std::string> keys, DispatcherId dispatcher_id) {
            if (!server::get().setting.dispatchers.is_valid(dispatcher_id)) {
              LOG_ERR("keybind: 'dispatcher' is invalid");
              return;
            }
            if (!this->try_register_keybind(keys, dispatcher_id)) {
              LOG_ERR("keybind: 'keys' might be invalid ({})", keys);
            }
            return;
          }));
}

void Keybinds::reset() {
  this->keybinds_.clear();
}

bool Keybinds::handle_key(xkb_keysym_t pressed_keysym, bool is_pressed,
    xkb_mod_mask_t modifier_mask) {
  // keybind has lower keysyms because of XKB_KEYSYM_CASE_INSENSITIVE
  pressed_keysym = xkb_keysym_to_lower(pressed_keysym);
  // FIXME: prevent sending release event by properly setting `handled`
  bool handled   = false;
  for (auto& state : this->keybinds_) {
    bool is_pressed_keysym_in_keybind = false;
    bool all_keys_pressed             = true;
    for (auto& key : state.keys) {
      if (key.first == pressed_keysym) {
        key.second                   = is_pressed;
        is_pressed_keysym_in_keybind = true;
      }
      all_keys_pressed &= key.second;
    }

    // fire only when the last key of the combination is pressed
    if (!is_pressed || !is_pressed_keysym_in_keybind || !all_keys_pressed) {
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
    const auto mod_mask = server::get()
                              .seat.keyboard.external_keyboard()
                              ->try_convert_to_mod_mask(key.c_str());
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
