#include "setting.hpp"

namespace yabe::core::setting {
Setting::Setting() {
  this->engine_.initialize([this](Engine* engine) {
    this->config.initialize(engine);
    this->dispatchers.initialize(engine);
    this->events.initialize(engine);
    this->keybinds.initialize(engine);
  });
}
void Setting::load_config(const char* config_contents) {
  this->dispatchers.reset();
  this->events.reset();
  this->keybinds.reset();
  this->engine_.do_string(config_contents);
}
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
std::string Setting::generate_metafile_text() const {
  return this->engine_.generate_metafile_text();
}
#endif
}  // namespace yabe::core::setting
