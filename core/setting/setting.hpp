#pragma once

#include "common/common.hpp"
#include "core/setting/fields/dispatchers.hpp"
#include "engine.hpp"
#include "fields/config.hpp"
#include "fields/keybinds.hpp"

namespace yabe::core::setting {
class Setting {
 public:
  DISABLE_MOVE_AND_COPY(Setting);
  explicit Setting();
  ~Setting() = default;

  void load_config(const char* config_contents);
  void add_error_message(std::string&& message);

  fields::Config      config;
  fields::Dispatchers dispatchers;
  fields::Keybinds    keybinds;

 private:
  Engine engine_;
};
}  // namespace yabe::core::setting
