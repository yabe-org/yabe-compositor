#pragma once

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "common/is-vector.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::schema {
inline std::string metafile_pascal_case(std::string_view name) {
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

template <typename T>
std::string metafile_type_name() {
  if constexpr (common::IsVector<T>::value) {
    return metafile_type_name<typename common::IsVector<T>::value_type>() +
           "[]";
  } else if constexpr (std::is_same_v<T, int>) {
    return "integer";
  } else if constexpr (std::is_same_v<T, double>) {
    return "number";
  } else if constexpr (std::is_same_v<T, std::string>) {
    return "string";
  } else if constexpr (std::is_same_v<T, void>) {
    return "nil";
  } else {
    static_assert(false, "Unsupported type for Lua metafile");
  }
}

template <typename... Fields>
std::string generate_table_metafile_text(
    const Table<Fields...>& table, const std::string& class_name) {
  std::stringstream result;
  result << "---@class " << class_name << '\n';
  std::apply(
      [&result, &class_name](const auto&... fields) {
        auto write_field = [&result, &class_name](const auto& field) {
          using Field = std::remove_cvref_t<decltype(field)>;
          result << "---@field " << static_cast<std::string_view>(Field::kName)
                 << "? ";
          if constexpr (requires { field.type; }) {
            result << class_name
                   << metafile_pascal_case(Field::kName.value.data());
          } else {
            result << metafile_type_name<typename Field::ValueType>();
          }
          if (!field.description.empty()) {
            result << " # " << field.description;
          }
          result << '\n';
        };
        (write_field(fields), ...);
      },
      table.fields);
  result << '\n';

  std::apply(
      [&result, &class_name](const auto&... fields) {
        auto write_nested_table = [&result, &class_name](const auto& field) {
          using Field = std::remove_cvref_t<decltype(field)>;
          if constexpr (requires { field.type; }) {
            result << generate_table_metafile_text(field.type,
                class_name + metafile_pascal_case(Field::kName.value.data()));
          }
        };
        (write_nested_table(fields), ...);
      },
      table.fields);
  return result.str();
}
}  // namespace yabe::core::setting::schema
