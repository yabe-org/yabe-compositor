#pragma once

#include <unordered_map>

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/function-entry.hpp"
#include "core/setting/schema/lua-function.hpp"

namespace yabe::core::setting::fields {
enum class Event {
  kStartup,
  kConfigReloaded,
  kWorkspaceChanged,
  kRecentered,
};

/// defines the Lua field name, description, and callback signature
template <Event>
struct EventTraits;
template <>
struct EventTraits<Event::kStartup> {
  static constexpr const char* kName = "startup";
  static constexpr const char* kDescription =
      "Register a callback called every time the compositor starts";
  using Callback = schema::LuaFunction<>;
};
template <>
struct EventTraits<Event::kConfigReloaded> {
  static constexpr const char* kName = "config_reloaded";
  static constexpr const char* kDescription =
      "Register a callback called when the config is reloaded "
      "while the compositor is running";
  using Callback = schema::LuaFunction<>;
};
template <>
struct EventTraits<Event::kWorkspaceChanged> {
  static constexpr const char* kName = "workspace_changed";
  static constexpr const char* kDescription =
      "Register a callback called when the current workspace is changed";
  using Callback = schema::LuaFunction<schema::Parameter<"previous", int>,
      schema::Parameter<"current", int>>;
};
template <>
struct EventTraits<Event::kRecentered> {
  static constexpr const char* kName = "recentered";
  static constexpr const char* kDescription =
      "Register a callback called when the front direction is reset";
  using Callback = schema::LuaFunction<>;
};

class Events {
 public:
  DISABLE_MOVE_AND_COPY(Events);
  explicit Events() = default;
  ~Events()         = default;

  void initialize(Engine* engine);
  void reset();

  template <Event E, typename... Args>
  void emit(const Args&... args) {
    using Arguments = typename EventTraits<E>::Callback::Arguments;
    static_assert(std::is_same_v<Arguments, std::tuple<Args...>>,
        "Arguments do not match the callback signature of the event");
    const auto it = this->handlers_.find(E);
    if (it == this->handlers_.end()) {
      return;
    }
    this->engine_->call_function(it->second, args...);
  }

 private:
  template <Event E>
  void create_register_function(Engine* engine) {
    using Traits   = EventTraits<E>;
    using Callback = typename Traits::Callback;
    engine->create_field(Traits::kName,
        schema::FunctionEntry<void(Callback)>::create(
            {
                .description     = Traits::kDescription,
                .parameter_names = {"callback"},
            },
            [this](Callback callback) {
              if (!this->handlers_.try_emplace(E, callback.ref).second) {
                this->engine_->unref_function(callback.ref);
                LOG_ERR("on.{}: callback is already registered; ignored",
                    Traits::kName);
              }
            }));
  }

  Engine*                        engine_ = nullptr;
  /// reference to the Lua function for each event
  std::unordered_map<Event, int> handlers_;
};
}  // namespace yabe::core::setting::fields
