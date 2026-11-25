#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace yabe::core::setting::reference {
struct Parameter;

struct Type {
  enum class Kind : uint8_t {
    NIL,
    BOOLEAN,
    INTEGER,
    NUMBER,
    STRING,
    DISPATCHER,
    ARRAY,
    FUNCTION,
    TABLE,
  };
  Kind                   kind;
  /// ARRAY: the element type (exactly one)
  std::vector<Type>      element;
  /// FUNCTION: parameters of the callback
  std::vector<Parameter> parameters;
  /// TABLE: name of the table in Reference::tables
  std::string            table_name;
};

struct Parameter {
  std::string name;
  Type        type;
};

using DefaultValue = std::variant<bool, int, double, std::string>;

struct TableField {
  std::string                 name;
  /// dot-separated keys from the table passed to the function
  std::string                 key_path;
  Type                        type;
  std::string                 description;
  /// nullopt for a nested table
  std::optional<DefaultValue> default_value;
};

struct Table {
  /// unique name used as the class name of LuaLS
  std::string             name;
  /// dot-separated keys from the table passed to the function
  /// (empty for the table passed to the function itself)
  std::string             key_path;
  std::vector<TableField> fields;
};

struct Function {
  std::string            name;
  std::string            description;
  std::vector<Parameter> parameters;
  Type                   return_type;
};

/// a namespace (Lua table) nested in another namespace
struct NamespaceLink {
  std::string name;
  std::string path;
  std::string class_name;
};

/// an integer value defined in a namespace
struct Constant {
  std::string name;
};

using NamespaceEntry = std::variant<Function, NamespaceLink, Constant>;

struct Namespace {
  /// dot-separated path from the global (e.g. `yabe.dispatchers`)
  std::string                 path;
  /// unique name used as the class name of LuaLS
  std::string                 class_name;
  /// in the order of definition
  std::vector<NamespaceEntry> entries;
};

struct Reference {
  /// the first one is the global namespace; the order of definition
  std::vector<Namespace> namespaces;
  /// parent tables are placed before their children
  std::vector<Table>     tables;
};
}  // namespace yabe::core::setting::reference
