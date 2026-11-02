#include "events.hpp"

namespace yabe::core::setting::fields {
void Events::initialize(Engine* engine) {
  this->engine_ = engine;
  engine->create_table("on", [this](Engine* engine) {
    this->create_register_function<Event::kStartup>(engine);
    this->create_register_function<Event::kConfigReloaded>(engine);
    this->create_register_function<Event::kWorkspaceChanged>(engine);
    this->create_register_function<Event::kRecentered>(engine);
  });
}

void Events::reset() {
  for (const auto& [event, function_ref] : this->handlers_) {
    this->engine_->unref_function(function_ref);
  }
  this->handlers_.clear();
}
}  // namespace yabe::core::setting::fields
