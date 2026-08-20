#include "engine.hpp"

#include <lauxlib.h>

#include <cassert>
#include <functional>
#include <lua.hpp>

#include "common/common.hpp"
#include "core/setting/schema/function-entry.hpp"
#include "lua.h"

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
  initializer(this);
  assert(lua_istable(this->l_, -1));
  lua_setglobal(this->l_, "yabe");
}

void Engine::do_string(const char* config_contents) {
  this->error_messages_.clear();
  if (luaL_dostring(this->l_, config_contents) != LUA_OK) {
    this->add_error_message(lua_tostring(this->l_, -1));
    lua_pop(this->l_, 1);
  }
  if (!this->error_messages_.empty()) {
    LOG_ERR(">>> Lua error(s):");
    for (const auto& message : this->error_messages_) {
      LOG_ERR("{}", message);
    }
    LOG_ERR(">>> Lua error(s) finished");
  }
}

void Engine::add_error_message(std::string&& message) {
  this->error_messages_.emplace_back(std::move(message));
}

void Engine::create_table(const char*   name,
    const std::function<void(Engine*)>& handle_initialize_table) {
  lua_newtable(this->l_);
  handle_initialize_table(this);
  // -1 is the field that will be set to the table
  lua_setfield(this->l_, -2, name);
}

void Engine::create_field(const char* name, int value) {
  assert(lua_istable(this->l_, -1));
  lua_pushinteger(this->l_, value);
  lua_setfield(this->l_, -2, name);
}
void Engine::create_field(
    const char* name, std::unique_ptr<schema::FunctionEntryBase>&& handler) {
  assert(lua_istable(this->l_, -1));
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
}  // namespace yabe::core::setting
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, bugprone-casting-through-void)
