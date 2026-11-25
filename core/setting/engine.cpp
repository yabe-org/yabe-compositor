#include "engine.hpp"

#include <cassert>
#include <functional>
#include <lua.hpp>

#include "common/common.hpp"
#include "core/setting/schema/describe.hpp"  // IWYU pragma: keep
#include "core/setting/schema/function-entry.hpp"

// Lua macro uses these features
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, bugprone-casting-through-void)

namespace yabe::core::setting {
namespace {
constexpr const char* kGlobalName = "yabe";
}  // namespace

Engine::Engine() {
  this->l_ = luaL_newstate();
  luaL_openlibs(this->l_);
  *static_cast<Engine**>(lua_getextraspace(this->l_)) = this;
}
Engine::~Engine() {
  lua_close(this->l_);
}

void Engine::initialize(const std::function<void(Engine*)>& initializer) {
  lua_newtable(this->l_);
  this->push_reference_namespace(nullptr);
  initializer(this);
  this->pop_reference_namespace();
  assert(lua_istable(this->l_, -1));
  lua_setglobal(this->l_, kGlobalName);
}

void Engine::do_string(const char* config_contents) {
  if (luaL_dostring(this->l_, config_contents) != LUA_OK) {
    LOG_ERR("{}", lua_tostring(this->l_, -1));
    lua_pop(this->l_, 1);
  }
}

int Engine::push_function(int function_ref) {
  lua_pushcfunction(this->l_, [](lua_State* l) -> int {
    const auto* const message = lua_tostring(l, 1);
    luaL_traceback(
        l, l, message ? message : "(error object is not a string)", 1);
    return 1;
  });
  const int message_handler_index = lua_gettop(this->l_);
  lua_rawgeti(this->l_, LUA_REGISTRYINDEX, function_ref);
  return message_handler_index;
}

void Engine::call_pushed_function(int num_args, int message_handler_index) {
  if (lua_pcall(this->l_, num_args, 0, message_handler_index) != LUA_OK) {
    LOG_ERR("Lua error: {}", lua_tostring(this->l_, -1));
    lua_pop(this->l_, 1);
  }
  lua_pop(this->l_, 1);
}

void Engine::unref_function(int function_ref) {
  luaL_unref(this->l_, LUA_REGISTRYINDEX, function_ref);
}

void Engine::create_table(const char*   name,
    const std::function<void(Engine*)>& handle_initialize_table) {
  lua_newtable(this->l_);
  this->push_reference_namespace(name);
  handle_initialize_table(this);
  this->pop_reference_namespace();
  // -1 is the field that will be set to the table
  lua_setfield(this->l_, -2, name);
}

void Engine::create_field(const char* name, int value) {
  assert(lua_istable(this->l_, -1));
  lua_pushinteger(this->l_, value);
  lua_setfield(this->l_, -2, name);
  this->add_reference_constant(name);
}
void Engine::create_field(
    const char* name, std::unique_ptr<schema::FunctionEntryBase>&& handler) {
  assert(lua_istable(this->l_, -1));
  this->add_reference_function(name, *handler);
  lua_pushlightuserdata(this->l_, handler.get());
  this->handlers_.emplace_back(std::move(handler));

  lua_pushcclosure(
      this->l_,
      [](lua_State* l) -> int {
        auto* engine  = *static_cast<Engine**>(lua_getextraspace(l));
        auto* handler = static_cast<schema::FunctionEntryBase*>(
            lua_touserdata(l, lua_upvalueindex(1)));
        return handler->invoke(engine);
      },
      1);
  lua_setfield(this->l_, -2, name);
}

void Engine::push_stack_value(int value) {
  lua_pushinteger(this->l_, value);
}
void Engine::push_stack_value(schema::DispatcherId value) {
  lua_pushinteger(this->l_, value.id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
/// `name` is nullptr for the global namespace
void Engine::push_reference_namespace(const char* name) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  if (name == nullptr) {
    this->reference_.namespaces.push_back({
        .path       = kGlobalName,
        .class_name = "YabeObject",
        .entries    = {},
    });
    this->reference_namespace_stack_.push_back({
        .index       = this->reference_.namespaces.size() - 1,
        .name_prefix = "Yabe",
    });
    return;
  }

  const auto& parent_frame = this->reference_namespace_stack_.back();
  const auto name_prefix = parent_frame.name_prefix + schema::pascal_case(name);
  reference::NamespaceLink link{
      .name = name,
      .path = this->reference_.namespaces[parent_frame.index].path + '.' + name,
      .class_name = name_prefix + "Namespace",
  };
  this->reference_.namespaces.push_back({
      .path       = link.path,
      .class_name = link.class_name,
      .entries    = {},
  });
  this->reference_.namespaces[parent_frame.index].entries.emplace_back(
      std::move(link));
  this->reference_namespace_stack_.push_back({
      .index       = this->reference_.namespaces.size() - 1,
      .name_prefix = name_prefix,
  });
#else
  static_cast<void>(name);
#endif
}
void Engine::pop_reference_namespace() {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  this->reference_namespace_stack_.pop_back();
#endif
}

void Engine::add_reference_constant(const char* name) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  const auto& frame = this->reference_namespace_stack_.back();
  this->reference_.namespaces[frame.index].entries.emplace_back(
      reference::Constant{.name = name});
#else
  static_cast<void>(name);
#endif
}

void Engine::add_reference_function(
    const char* name, const schema::FunctionEntryBase& handler) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  const auto& frame    = this->reference_namespace_stack_.back();
  auto        function = handler.describe(name,
      frame.name_prefix + schema::pascal_case(name), this->reference_.tables);
  this->reference_.namespaces[frame.index].entries.emplace_back(
      std::move(function));
#else
  static_cast<void>(name);
  static_cast<void>(handler);
#endif
}
// NOLINTEND(readability-convert-member-functions-to-static)
}  // namespace yabe::core::setting
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, bugprone-casting-through-void)
