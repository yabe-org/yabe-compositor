#include "dispatchers.hpp"

#include <cassert>

#include "common/common.hpp"
#include "core/setting/schema/function-entry.hpp"

namespace yabe::core::setting::fields {
DispatcherId Dispatchers::next_dispatcher_id = 0;

void Dispatchers::set_executer(
    const std::function<int(const char* const *)>&& execvp) {
  this->execvp_ = execvp;
}

void Dispatchers::initialize(Engine* engine) {
  engine->create_table("dispatchers", [this](Engine* engine) {
    engine->create_field("exec",
        schema::FunctionEntry<int(std::string)>::create(
            {
                .description     = "Execute a shell command via `sh -c`",
                .parameter_names = {"command"},
            },
            [this](std::string command) {
              LOG_DBG("dispatchers.exec: {}", command);
              this->dispatchers_.emplace_back([this, command = command]() {
                const std::vector<const char*> argv{
                    "sh", "-c", command.c_str(), nullptr};
                this->execvp_(argv.data());
              });
              const auto dispatcher_id = Dispatchers::get_next_dispatcher_id();
              assert(static_cast<size_t>(Dispatchers::next_dispatcher_id) ==
                     this->dispatchers_.size());
              return dispatcher_id;
            }));
  });
}

void Dispatchers::reset() {
  this->dispatchers_.clear();
  Dispatchers::next_dispatcher_id = 0;
}

void Dispatchers::dispatch(DispatcherId id) {
  if (id < 0 || static_cast<size_t>(id) >= this->dispatchers_.size()) {
    LOG_ERR("Invalid dispatcher id: {}", id);
    return;
  }
  this->dispatchers_[id]();
}

DispatcherId Dispatchers::get_next_dispatcher_id() {
  return Dispatchers::next_dispatcher_id++;
}
}  // namespace yabe::core::setting::fields
