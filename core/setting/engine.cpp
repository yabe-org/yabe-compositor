#include "engine.hpp"

#include <cassert>
#include <functional>
#include <lua.hpp>

#include "common/common.hpp"
#include "core/setting/schema/function-entry.hpp"
#include "core/setting/schema/metafile.hpp"  // IWYU pragma: keep

// Lua macro uses these features
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, bugprone-casting-through-void)

namespace yabe::core::setting {
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
  this->push_metafile_table(nullptr);
  initializer(this);
  this->pop_metafile_table();
  assert(lua_istable(this->l_, -1));
  lua_setglobal(this->l_, "yabe");
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
  this->push_metafile_table(name);
  handle_initialize_table(this);
  this->pop_metafile_table();
  // -1 is the field that will be set to the table
  lua_setfield(this->l_, -2, name);
}

void Engine::create_field(const char* name, int value) {
  assert(lua_istable(this->l_, -1));
  lua_pushinteger(this->l_, value);
  lua_setfield(this->l_, -2, name);
  this->add_metafile_field(name);
}
void Engine::create_field(
    const char* name, std::unique_ptr<schema::FunctionEntryBase>&& handler) {
  assert(lua_istable(this->l_, -1));
  this->add_metafile_field(name, *handler);
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

// NOLINTBEGIN(readability-convert-member-functions-to-static)
void Engine::push_metafile_table(const char* name) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  if (name == nullptr) {
    this->metafile_tables_.push_back({
        .class_name_prefix = "Yabe",
        .class_name        = "YabeObject",
        .fields            = {},
    });
    this->metafile_table_stack_.push_back(0);
    return;
  }

  const auto& parent =
      this->metafile_tables_[this->metafile_table_stack_.back()];
  const auto class_name_prefix =
      parent.class_name_prefix + schema::metafile_pascal_case(name);
  const auto class_name = class_name_prefix + "Namespace";
  this->metafile_tables_[this->metafile_table_stack_.back()].fields.push_back({
      .name        = name,
      .type        = class_name,
      .description = {},
  });
  this->metafile_tables_.push_back({
      .class_name_prefix = class_name_prefix,
      .class_name        = class_name,
      .fields            = {},
  });
  this->metafile_table_stack_.push_back(this->metafile_tables_.size() - 1);
#else
  static_cast<void>(name);
#endif
}
void Engine::pop_metafile_table() {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  this->metafile_table_stack_.pop_back();
#endif
}

void Engine::add_metafile_field(const char* name) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  this->metafile_tables_[this->metafile_table_stack_.back()].fields.push_back({
      .name        = name,
      .type        = "integer",
      .description = {},
  });
#else
  static_cast<void>(name);
#endif
}

void Engine::add_metafile_field(
    const char* name, const schema::FunctionEntryBase& handler) {
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  const auto& table =
      this->metafile_tables_[this->metafile_table_stack_.back()];
  auto metafile = handler.generate_metafile_text(
      table.class_name_prefix + schema::metafile_pascal_case(name));
  this->metafile_tables_[this->metafile_table_stack_.back()].fields.push_back({
      .name        = name,
      .type        = std::move(metafile.type),
      .description = std::move(metafile.description),
  });
  if (!metafile.definitions.empty()) {
    this->metafile_definitions_.push_back(std::move(metafile.definitions));
  }
#else
  static_cast<void>(name);
  static_cast<void>(handler);
#endif
}
// NOLINTEND(readability-convert-member-functions-to-static)
}  // namespace yabe::core::setting
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, bugprone-casting-through-void)
