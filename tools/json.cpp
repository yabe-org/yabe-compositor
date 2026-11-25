#include "json.hpp"

#include <cmath>
#include <format>
#include <utility>

namespace yabe::tools {
namespace {
void write_indent(std::string& out, std::size_t depth) {
  out.append(depth * 2, ' ');
}

void write_string(std::string& out, const std::string& value) {
  out += '"';
  for (const auto character : value) {
    switch (character) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(character) < 0x20) {
          out +=
              std::format("\\u{:04x}", static_cast<unsigned char>(character));
        } else {
          out += character;
        }
    }
  }
  out += '"';
}
}  // namespace

JsonValue::JsonValue(std::nullptr_t value) : value_(value) {
}
JsonValue::JsonValue(bool value) : value_(value) {
}
JsonValue::JsonValue(int value) : value_(value) {
}
JsonValue::JsonValue(double value) : value_(value) {
}
JsonValue::JsonValue(const char* value) : value_(std::string(value)) {
}
JsonValue::JsonValue(std::string value) : value_(std::move(value)) {
}
JsonValue::JsonValue(JsonArray value) : value_(std::move(value)) {
}
JsonValue::JsonValue(JsonObject value) : value_(std::move(value)) {
}

std::string JsonValue::dump() const {
  std::string result;
  this->dump(result, 0);
  result += '\n';
  return result;
}

void JsonValue::dump(std::string& out, std::size_t depth) const {
  if (std::holds_alternative<std::nullptr_t>(this->value_)) {
    out += "null";
  } else if (const auto* value = std::get_if<bool>(&this->value_)) {
    out += *value ? "true" : "false";
  } else if (const auto* value = std::get_if<int>(&this->value_)) {
    out += std::to_string(*value);
  } else if (const auto* value = std::get_if<double>(&this->value_)) {
    // JSON does not have representation of them
    out += std::isfinite(*value) ? std::format("{}", *value) : "null";
  } else if (const auto* value = std::get_if<std::string>(&this->value_)) {
    write_string(out, *value);
  } else if (const auto* value = std::get_if<JsonArray>(&this->value_)) {
    if (value->empty()) {
      out += "[]";
      return;
    }
    out += '[';
    for (const auto& element : *value) {
      out += &element == &value->front() ? "\n" : ",\n";
      write_indent(out, depth + 1);
      element.dump(out, depth + 1);
    }
    out += '\n';
    write_indent(out, depth);
    out += ']';
  } else if (const auto* value = std::get_if<JsonObject>(&this->value_)) {
    if (value->empty()) {
      out += "{}";
      return;
    }
    out += '{';
    for (const auto& member : *value) {
      out += &member == &value->front() ? "\n" : ",\n";
      write_indent(out, depth + 1);
      write_string(out, member.key);
      out += ": ";
      member.value.dump(out, depth + 1);
    }
    out += '\n';
    write_indent(out, depth);
    out += '}';
  }
}
}  // namespace yabe::tools
