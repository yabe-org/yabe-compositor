#pragma once

#include <cassert>
#include <functional>
#include <lua.hpp>
#include <memory>
#include <sstream>  // IWYU pragma: keep
#include <string>
#include <vector>

#include "common/common.hpp"
#include "common/is-vector.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting {
namespace schema {
class FunctionEntryBase;
}  // namespace schema

// to avoid header name collision (lua.hpp), it is named Engine
// but actually it is only for Lua
class Engine {
 public:
  DISABLE_MOVE_AND_COPY(Engine);
  explicit Engine();
  ~Engine();

  void initialize(const std::function<void(Engine*)>& initializer);
  void do_string(const char* config_contents);
  void add_error_message(std::string&& message);

  void create_table(const char*           name,
      const std::function<void(Engine*)>& handle_initialize_table);

  void create_field(const char* name, int value);
  void create_field(
      const char* name, std::unique_ptr<schema::FunctionEntryBase>&& handler);

  void push_stack_value(int value);

  template <typename T>
  std::optional<T> get_stack_value(int stack_index) {
    std::function<int(lua_State*, int)>     checker;
    std::function<T(lua_State*, int, int*)> getter;
    // this vector branch does not use checker / getter,
    // but this check should be here to avoid compilation error
    if constexpr (common::IsVector<T>::value) {
      if (!lua_istable(this->l_, stack_index)) {
        return std::nullopt;
      }
      using ValueType = typename common::IsVector<T>::value_type;
      // using T disables syntax highlight and completion
      std::vector<ValueType> result;
      auto                   len = luaL_len(this->l_, stack_index);
      for (lua_Integer i = 1; i <= len; ++i) {
        lua_geti(this->l_, stack_index, i);
        auto value = this->get_stack_value<ValueType>(-1);
        lua_pop(this->l_, 1);
        if (!value) {
          return std::nullopt;
        }
        result.push_back(*value);
      }
      return result;
    } else if constexpr (std::is_same_v<T, int>) {
      checker = lua_isinteger;
      getter  = lua_tointegerx;
    } else if constexpr (std::is_same_v<T, double>) {
      checker = lua_isnumber;
      getter  = lua_tonumberx;
    } else if constexpr (std::is_same_v<T, std::string>) {
      checker = lua_isstring;
      getter  = [](lua_State* l, int index, const int* /*len*/) {
        const auto* const str = lua_tostring(l, index);
        return str ? std::string(str) : std::string();
      };
    } else {
      static_assert(false, "Unsupported type for get_field");
    }

    std::optional<T> result = std::nullopt;
    if (checker(this->l_, stack_index)) {
      result = getter(this->l_, stack_index, nullptr);
    }
    return result;
  }

  template <typename T>
  std::optional<T> get_table_field(int table_index, const char* field_name) {
    if (!lua_istable(this->l_, table_index)) {
      return std::nullopt;
    }
    lua_getfield(this->l_, table_index, field_name);
    auto result = this->get_stack_value<T>(-1);
    lua_pop(this->l_, 1);
    return result;
  }

  template <typename... Fields>
  std::optional<schema::TableValue<schema::Table<Fields...>>> get_stack_value(
      int stack_index, const schema::Table<Fields...>& schema) {
    if (!lua_istable(this->l_, stack_index)) {
      return std::nullopt;
    }
    stack_index = lua_absindex(this->l_, stack_index);

    schema::TableValue<schema::Table<Fields...>> result{schema};
    bool                                         is_valid = true;
    std::apply(
        [this, stack_index, &result, &is_valid](const auto&... fields) {
          auto read_field = [this, stack_index, &result, &is_valid](
                                const auto& field) {
            using Field = std::remove_cvref_t<decltype(field)>;
            lua_getfield(this->l_, stack_index, Field::kName.value.data());
            if (lua_isnil(this->l_, -1)) {
              lua_pop(this->l_, 1);
              return;
            }

            if constexpr (requires { field.type; }) {
              auto value = this->get_stack_value(-1, field.type);
              if (value) {
                result.template set<Field::kName>(std::move(*value));
              } else {
                is_valid = false;
              }
            } else {
              auto value = this->get_stack_value<typename Field::ValueType>(-1);
              if (value) {
                result.template set<Field::kName>(std::move(*value));
              } else {
                is_valid = false;
              }
            }
            lua_pop(this->l_, 1);
          };
          (read_field(fields), ...);
        },
        schema.fields);
    if (!is_valid) {
      return std::nullopt;
    }
    return result;
  }

 private:
  void push_metafile_table(const char* name);
  void pop_metafile_table();
  void add_metafile_field(const char* name);
  void add_metafile_field(
      const char* name, const schema::FunctionEntryBase& handler);

  lua_State*                                              l_;
  std::vector<std::unique_ptr<schema::FunctionEntryBase>> handlers_;
  std::vector<std::string>                                error_messages_;

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
 public:
  std::string generate_metafile_text() const {
    std::stringstream result;
    result << "---@meta\n\n";
    for (const auto& table : this->metafile_tables_) {
      result << "---@class " << table.class_name << '\n';
      for (const auto& field : table.fields) {
        result << "---@field " << field.name << ' ' << field.type;
        if (!field.description.empty()) {
          result << " # " << field.description;
        }
        result << '\n';
      }
      result << '\n';
    }
    for (const auto& definitions : this->metafile_definitions_) {
      result << definitions;
    }
    result << "---@type YabeObject\n" << "yabe = {}\n";
    return result.str();
  }

 private:
  struct MetafileField {
    std::string name;
    std::string type;
    std::string description;
  };
  struct MetafileTable {
    std::string                class_name_prefix;
    std::string                class_name;
    std::vector<MetafileField> fields;
  };
  std::vector<MetafileTable> metafile_tables_;
  std::vector<std::size_t>   metafile_table_stack_;
  std::vector<std::string>   metafile_definitions_;
#endif
};
}  // namespace yabe::core::setting
