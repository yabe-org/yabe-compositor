#pragma once

#include "common/common.hpp"
#include "core/setting/fields/dispatchers.hpp"
#include "engine.hpp"
#include "fields/config.hpp"
#include "fields/events.hpp"
#include "fields/keybinds.hpp"

namespace yabe::core::setting {
class Setting {
 public:
  DISABLE_MOVE_AND_COPY(Setting);
  explicit Setting();
  ~Setting() = default;

  void load_config(const char* config_contents);

  fields::Config      config;
  fields::Dispatchers dispatchers;
  fields::Events      events;
  fields::Keybinds    keybinds;

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  std::string generate_metafile_text() const;
#endif

 private:
  Engine engine_;
};
}  // namespace yabe::core::setting
