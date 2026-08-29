#pragma once

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::fields {
// clang-format off
constexpr auto kConfigTable = schema::table(
  schema::field<"input">(schema::table(
    schema::field<"pointer">(schema::table(
      schema::field<"movement_multiplier", double>(
         "This value will be multiplied to the amount of pointer movement",
         1.0
      ),
      schema::field<"scroll_multiplier", double>(
         "This value will be multiplied to the amount of pointer scroll",
         1.0
      )
    )), // pointer
    schema::field<"keyboard">(schema::table(
      schema::field<"rules", std::string>(
         "keyboard rules (XKB)",
         ""
      ),
      schema::field<"model", std::string>(
         "keyboard model (XKB)",
         ""
      ),
      schema::field<"layout", std::string>(
         "keyboard layout (XKB)",
         "us"
      ),
      schema::field<"variant", std::string>(
         "keyboard variant (XKB)",
         ""
      ),
      schema::field<"options", std::string>(
         "keyboard options (XKB)",
         ""
      )
    )) // keyboard
  )), // input
  schema::field<"pixels_per_meter", double>(
     "Number of pixels per meter in the world",
     2500.0
  )
);
using ConfigSchema = std::remove_cv_t<decltype(kConfigTable)>;
// clang-format on

class Config {
 public:
  DISABLE_MOVE_AND_COPY(Config);
  explicit Config() = default;
  ~Config()         = default;

  void initialize(Engine* engine);

  template <schema::FixedString... Path>
  decltype(auto) get() const {
    return this->values_.get<Path...>();
  }

 private:
  void on_config_changed() const;

  schema::TableValue<ConfigSchema> values_{kConfigTable};
};
}  // namespace yabe::core::setting::fields
