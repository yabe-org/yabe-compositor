#pragma once

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::fields {
// clang-format off
constexpr auto kMargin = schema::table(
  schema::field<"top", int>(
     "top margin",
     0
  ),
  schema::field<"bottom", int>(
     "bottom margin",
     0
  ),
  schema::field<"left", int>(
     "left margin",
     0
  ),
  schema::field<"right", int>(
     "right margin",
     0
  ),
  schema::field<"x", int>(
     "left and right margin",
     0
  ),
  schema::field<"y", int>(
     "top and bottom margin",
     0
  ),
  schema::field<"rest", int>(
     "margin that is not specified",
     0
  )
);
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
  schema::field<"surface_grouping">(schema::table(
      schema::field<"enabled", bool>(
         "Enable surface grouping feature",
         false
      ),
      schema::field<"fadeout_duration_ms", int>(
         "Duration in milliseconds for the lasso to fade out after selection",
         300
      ),
      schema::field<"line_width", double>(
         "line width of the lasso while selecting",
         3.0
      ),
      schema::field<"line_distance", double>(
         "how far the lasso is from the camera",
         0.9
      ),
      schema::field<"window_spacing", double>(
         "space between windows in a group",
         0.008
      ),
      schema::field<"padding", double>(
         "padding around the group",
         0.01
      ),
      schema::field<"detach_threshold", double>(
         "the amount of triggering children detach",
         0.65
      )
  )),
  schema::field<"window">(schema::table(
      schema::field<"resize_handle_thickness", int>(
         "line width of the resize handle (px)",
         5
      ),
      schema::field<"resize_handle_threshold", int>(
         "length to activate the resize handle (px). set zero to disable",
         50
      )
  )),
  schema::field<"decoration">(schema::table(
      schema::field<"height", int>(
         "height of the decoration bar (px)",
         50
      ),
      schema::field<"offset", int>(
         "space between contents (ex: window) and the decoration (px)",
         45
      )
  )),
  schema::field<"fixed_layer">(schema::table(
      schema::field<"margin">(kMargin),
      schema::field<"follow_speed", double>(
         "how fast the head-fixed layer follows the user's head direction",
         7.5
      )
  )),
  schema::field<"auto_reload_config", bool>(
     "Automatically reload config file if it is changed",
     true
  ),
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
