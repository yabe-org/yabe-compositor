#include "config.hpp"

#include "core/server.hpp"
#include "core/setting/schema/function-entry.hpp"

namespace yabe::core::setting::fields {
void Config::initialize(Engine* engine) {
  engine->create_field(
      "config", schema::FunctionEntry<void(ConfigSchema)>::create(kConfigTable,
                    {
                        .description     = "Set the configuration",
                        .parameter_names = {"config"},
                    },
                    [this](schema::TableValue<ConfigSchema> config) {
                      this->values_ = std::move(config);
                      this->on_config_changed();
                    }));
}

void Config::on_config_changed() const {
  server::get().seat.keyboard.init_keymap(
      this->get<"input", "keyboard", "rules">().c_str(),
      this->get<"input", "keyboard", "model">().c_str(),
      this->get<"input", "keyboard", "layout">().c_str(),
      this->get<"input", "keyboard", "variant">().c_str(),
      this->get<"input", "keyboard", "options">().c_str());
  server::get().layer_manager.reconfigure_all();
}
}  // namespace yabe::core::setting::fields
