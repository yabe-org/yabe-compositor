#include "reference-json.hpp"

#include <variant>

#include "json.hpp"
#include "lua-meta-file.hpp"

namespace yabe::tools {
namespace reference = core::setting::reference;

namespace {
const char* kind_name(reference::Type::Kind kind) {
  using Kind = reference::Type::Kind;
  switch (kind) {
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
      return "dispatcher";
    case Kind::ARRAY:
      return "array";
    case Kind::FUNCTION:
      return "function";
    case Kind::TABLE:
      return "table";
  }
  return "unknown";
}

JsonValue to_json(const reference::Type& type);

JsonValue to_json(const std::vector<reference::Parameter>& parameters) {
  JsonArray result;
  for (const auto& parameter : parameters) {
    result.emplace_back(JsonObject{
        {.key = "name", .value = parameter.name         },
        {.key = "type", .value = to_json(parameter.type)},
    });
  }
  return result;
}

JsonValue to_json(const reference::Type& type) {
  JsonObject result{
      {.key = "kind",  .value = kind_name(type.kind)},
      {.key = "label", .value = lua_type_name(type) },
  };
  switch (type.kind) {
    case reference::Type::Kind::ARRAY:
      result.push_back(
          {.key = "element", .value = to_json(type.element.front())});
      break;
    case reference::Type::Kind::FUNCTION:
      result.push_back(
          {.key = "parameters", .value = to_json(type.parameters)});
      break;
    case reference::Type::Kind::TABLE:
      result.push_back({.key = "table", .value = type.table_name});
      break;
    default:
      break;
  }
  return result;
}

JsonValue to_json(const reference::DefaultValue& value) {
  return std::visit(
      [](const auto& value) {
        return JsonValue{value};
      },
      value);
}

std::string signature(
    const std::string& path, const reference::Function& function) {
  return path + '(' + lua_parameter_list(function.parameters) +
         "): " + lua_type_name(function.return_type);
}

JsonValue to_json(const reference::Namespace& ns) {
  JsonArray functions;
  JsonArray namespaces;
  JsonArray constants;
  for (const auto& entry : ns.entries) {
    if (const auto* function = std::get_if<reference::Function>(&entry)) {
      const auto path = ns.path + '.' + function->name;
      functions.emplace_back(JsonObject{
          {.key = "name", .value = function->name},
          {.key = "path", .value = path},
          {.key = "signature", .value = signature(path, *function)},
          {.key = "description", .value = function->description},
          {.key = "parameters", .value = to_json(function->parameters)},
          {.key = "returns", .value = to_json(function->return_type)},
      });
    } else if (const auto* link =
                   std::get_if<reference::NamespaceLink>(&entry)) {
      namespaces.emplace_back(link->path);
    } else if (const auto* constant =
                   std::get_if<reference::Constant>(&entry)) {
      constants.emplace_back(JsonObject{
          {.key = "name",      .value = constant->name                },
          {.key = "path",      .value = ns.path + '.' + constant->name},
          {.key      = "type",
           .value = to_json(reference::Type{
                  .kind       = reference::Type::Kind::INTEGER,
                  .element    = {},
                  .parameters = {},
                  .table_name = {},
              })                                                      },
      });
    }
  }
  return JsonObject{
      {.key = "path",       .value = ns.path              },
      {.key = "functions",  .value = std::move(functions) },
      {.key = "namespaces", .value = std::move(namespaces)},
      {.key = "constants",  .value = std::move(constants) },
  };
}

JsonValue to_json(const reference::Table& table) {
  JsonArray fields;
  for (const auto& field : table.fields) {
    JsonObject object{
        {.key = "name",        .value = field.name         },
        {.key = "keyPath",     .value = field.key_path     },
        {.key = "type",        .value = to_json(field.type)},
        {.key = "description", .value = field.description  },
    };
    if (field.default_value) {
      object.push_back(
          {.key = "default", .value = to_json(*field.default_value)});
    }
    fields.emplace_back(std::move(object));
  }
  return JsonObject{
      {.key = "name",    .value = table.name       },
      {.key = "keyPath", .value = table.key_path   },
      {.key = "fields",  .value = std::move(fields)},
  };
}
}  // namespace

std::string generate_reference_json(const reference::Reference& reference) {
  JsonObject namespaces;
  for (const auto& ns : reference.namespaces) {
    namespaces.push_back({.key = ns.path, .value = to_json(ns)});
  }
  JsonObject tables;
  for (const auto& table : reference.tables) {
    tables.push_back({.key = table.name, .value = to_json(table)});
  }
  return JsonValue{
      JsonObject{

                 {.key = "namespaces", .value = std::move(namespaces)},
                 {.key = "tables", .value = std::move(tables)},
                 }
  }
      .dump();
}
}  // namespace yabe::tools
