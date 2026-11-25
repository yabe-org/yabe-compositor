#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "common/is-vector.hpp"
#include "core/setting/reference.hpp"
#include "core/setting/schema/dispatcher-id.hpp"
#include "core/setting/schema/lua-function.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::schema {
inline std::string pascal_case(std::string_view name) {
  std::string result;
  bool        capitalize = true;
  for (const auto character : name) {
    if (character == '_') {
      capitalize = true;
      continue;
    }
    result += capitalize ? static_cast<char>(std::toupper(
                               static_cast<unsigned char>(character))) :
                           character;
    capitalize = false;
  }
  return result;
}

inline std::string join_key_path(
    const std::string& parent, std::string_view name) {
  return parent.empty() ? std::string(name) : parent + '.' + std::string(name);
}

template <typename T>
reference::Type describe_type();

template <typename... Parameters>
reference::Type describe_lua_function_type(
    std::type_identity<LuaFunction<Parameters...>> /*type*/) {
  return {
      .kind       = reference::Type::Kind::FUNCTION,
      .element    = {},
      .parameters = {reference::Parameter{
          .name = std::string(static_cast<std::string_view>(Parameters::kName)),
          .type = describe_type<typename Parameters::ValueType>(),
      }...},
      .table_name = {},
  };
}

template <typename T>
reference::Type describe_type() {
  using Kind = reference::Type::Kind;
  if constexpr (common::IsVector<T>::value) {
    return {
        .kind    = Kind::ARRAY,
        .element = {describe_type<typename common::IsVector<T>::value_type>()},
        .parameters = {},
        .table_name = {},
    };
  } else if constexpr (IsLuaFunction<T>::value) {
    return describe_lua_function_type(std::type_identity<T>{});
  } else {
    const auto kind = [] {
      if constexpr (std::is_same_v<T, bool>) {
        return Kind::BOOLEAN;
      } else if constexpr (std::is_same_v<T, int>) {
        return Kind::INTEGER;
      } else if constexpr (std::is_same_v<T, double>) {
        return Kind::NUMBER;
      } else if constexpr (std::is_same_v<T, std::string>) {
        return Kind::STRING;
      } else if constexpr (std::is_same_v<T, DispatcherId>) {
        return Kind::DISPATCHER;
      } else if constexpr (std::is_same_v<T, void>) {
        return Kind::NIL;
      } else {
        static_assert(false, "Unsupported type for the reference");
      }
    }();
    return {.kind = kind, .element = {}, .parameters = {}, .table_name = {}};
  }
}

inline reference::Type describe_table_type(std::string table_name) {
  return {
      .kind       = reference::Type::Kind::TABLE,
      .element    = {},
      .parameters = {},
      .table_name = std::move(table_name),
  };
}

/// @brief appends `table` and its nested tables to `tables`
///        (the parent is placed before its children)
/// @param name unique name of `table`
/// @param key_path dot-separated keys from the table passed to the function
template <typename... Fields>
void describe_table(const Table<Fields...>& table, const std::string& name,
    const std::string& key_path, std::vector<reference::Table>& tables) {
  reference::Table result{.name = name, .key_path = key_path, .fields = {}};
  std::apply(
      [&](const auto&... fields) {
        auto describe_field = [&](const auto& field) {
          using Field           = std::remove_cvref_t<decltype(field)>;
          const auto field_name = static_cast<std::string_view>(Field::kName);
          reference::TableField described{
              .name          = std::string(field_name),
              .key_path      = join_key_path(key_path, field_name),
              .type          = {},
              .description   = std::string(field.description),
              .default_value = std::nullopt,
          };
          if constexpr (requires { field.type; }) {
            described.type =
                describe_table_type(name + pascal_case(field_name));
          } else {
            using ValueType = Field::ValueType;
            static_assert(
                std::is_constructible_v<reference::DefaultValue, ValueType>,
                "Unsupported type for the default value of the reference");
            described.type = describe_type<ValueType>();
            described.default_value =
                reference::DefaultValue{ValueType{field.default_value}};
          }
          result.fields.push_back(std::move(described));
        };
        (describe_field(fields), ...);
      },
      table.fields);
  tables.push_back(std::move(result));

  std::apply(
      [&](const auto&... fields) {
        auto describe_nested_table = [&](const auto& field) {
          using Field = std::remove_cvref_t<decltype(field)>;
          if constexpr (requires { field.type; }) {
            const auto field_name = static_cast<std::string_view>(Field::kName);
            describe_table(field.type, name + pascal_case(field_name),
                join_key_path(key_path, field_name), tables);
          }
        };
        (describe_nested_table(fields), ...);
      },
      table.fields);
}
}  // namespace yabe::core::setting::schema
