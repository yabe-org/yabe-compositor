#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace yabe::tools {
class JsonValue;
struct JsonMember;
using JsonArray  = std::vector<JsonValue>;
/// members are written in the order of insertion
using JsonObject = std::vector<JsonMember>;

/// minimal JSON value to write the reference file
class JsonValue {
 public:
  using Variant = std::variant<std::nullptr_t, bool, int, double, std::string,
      JsonArray, JsonObject>;

  // NOLINTBEGIN(*-explicit-constructor)
  // to construct from each type implicitly (e.g. `JsonValue v = "str";`)
  JsonValue(std::nullptr_t value);
  JsonValue(bool value);
  JsonValue(int value);
  JsonValue(double value);
  JsonValue(const char* value);
  JsonValue(std::string value);
  JsonValue(JsonArray value);
  JsonValue(JsonObject value);
  // NOLINTEND(*-explicit-constructor)

  /// writes the value with 2-space indentation
  [[nodiscard]] std::string dump() const;

 private:
  void dump(std::string& out, std::size_t depth) const;

  Variant value_;
};

struct JsonMember {
  std::string key;
  JsonValue   value;
};
}  // namespace yabe::tools
