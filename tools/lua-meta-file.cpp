#include "lua-meta-file.hpp"

#include <cassert>
#include <sstream>
#include <variant>

namespace yabe::tools {
namespace reference = core::setting::reference;

namespace {
constexpr const char* kDispatcherTypeName = "YabeDispatcherID";
}  // namespace

std::string lua_type_name(const reference::Type& type) {
  using Kind = reference::Type::Kind;
  switch (type.kind) {
    case Kind::NIL:
      return "nil";
    case Kind::BOOLEAN:
      return "boolean";
    case Kind::INTEGER:
      return "integer";
    case Kind::NUMBER:
      return "number";
    case Kind::STRING:
      return "string";
    case Kind::DISPATCHER:
      return kDispatcherTypeName;
    case Kind::ARRAY:
      assert(type.element.size() == 1);
      return lua_type_name(type.element.front()) + "[]";
    case Kind::FUNCTION:
      return "fun(" + lua_parameter_list(type.parameters) + ")";
    case Kind::TABLE:
      return type.table_name;
  }
  assert(false && "unknown type kind");
  return {};
}

std::string lua_parameter_list(
    const std::vector<reference::Parameter>& parameters) {
  std::string result;
  for (const auto& parameter : parameters) {
    if (!result.empty()) {
      result += ", ";
    }
    result += parameter.name + ": " + lua_type_name(parameter.type);
  }
  return result;
}

std::string generate_lua_meta_file(const reference::Reference& reference) {
  std::stringstream result;
  result << "---@meta\n\n";
  result << "--- handle returned by `yabe.dispatchers.*`\n"
         << "---@alias " << kDispatcherTypeName << " integer\n\n";

  const auto write_field = [&result](const std::string& name, bool is_optional,
                               const std::string& type,
                               const std::string& description) {
    result << "---@field " << name << (is_optional ? "? " : " ") << type;
    if (!description.empty()) {
      result << " # " << description;
    }
    result << '\n';
  };

  for (const auto& ns : reference.namespaces) {
    result << "---@class " << ns.class_name << '\n';
    for (const auto& entry : ns.entries) {
      if (const auto* function = std::get_if<reference::Function>(&entry)) {
        write_field(function->name, false,
            "fun(" + lua_parameter_list(function->parameters) +
                "): " + lua_type_name(function->return_type),
            function->description);
      } else if (const auto* link =
                     std::get_if<reference::NamespaceLink>(&entry)) {
        write_field(link->name, false, link->class_name, "");
      } else if (const auto* constant =
                     std::get_if<reference::Constant>(&entry)) {
        write_field(constant->name, false, "integer", "");
      }
    }
    result << '\n';
  }

  for (const auto& table : reference.tables) {
    result << "---@class " << table.name << '\n';
    for (const auto& field : table.fields) {
      write_field(
          field.name, true, lua_type_name(field.type), field.description);
    }
    result << '\n';
  }

  if (!reference.namespaces.empty()) {
    const auto& global = reference.namespaces.front();
    result << "---@type " << global.class_name << '\n'
           << global.path << " = {}\n";
  }
  return result.str();
}
}  // namespace yabe::tools
