#include "dispatchers.hpp"

#include <cassert>

#include "common/common.hpp"
#include "core/server.hpp"
#include "core/setting/schema/function-entry.hpp"

namespace yabe::core::setting::fields {
DispatcherId Dispatchers::next_dispatcher_id = 0;

void Dispatchers::set_executer(
    const std::function<int(const char* const *)>&& execvp) {
  this->execvp_ = execvp;
}

void Dispatchers::initialize(Engine* engine) {
  engine->create_table("dispatchers", [this](Engine* engine) {
    engine->create_field("set_workspace",
        schema::FunctionEntry<int(int)>::create(
            {
                .description = "Set the current workspace by absolute index",
                .parameter_names = {"index"},
            },
            [this](int index) {
              if (index < 0 || index >= 10) {
                LOG_ERR("Invalid workspace index: {}", index);
                return -1;
              }
              this->dispatchers_.emplace_back([index]() {
                server::get().workspace.set_current_scene_index(index);
              });
              return Dispatchers::get_next_dispatcher_id();
            }));

    engine->create_field("add_workspace_index",
        schema::FunctionEntry<int(int)>::create(
            {
                .description =
                    "Add to the current workspace index (can be negative)",
                .parameter_names = {"delta"},
            },
            [this](int delta) {
              this->dispatchers_.emplace_back([delta]() {
                server::get().workspace.add_scene_index(delta);
              });
              return Dispatchers::get_next_dispatcher_id();
            }));

    engine->create_field("recenter",
        schema::FunctionEntry<int()>::create(
            {
                .description     = "Make the current head direction the front",
                .parameter_names = {},
            },
            [this]() {
              this->dispatchers_.emplace_back([]() {
                server::get().request_recenter();
              });
              return Dispatchers::get_next_dispatcher_id();
            }));

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
              return Dispatchers::get_next_dispatcher_id();
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
