#pragma once

#include <cstdint>

namespace yabe::core::unit {
struct Pixel;
struct WorldUnit {
  float value;
  float operator*() const {
    return this->value;
  }
  [[nodiscard]] Pixel to_pixel() const;
};

struct Pixel {
  int32_t value;
  int32_t operator*() const {
    return this->value;
  }
  [[nodiscard]] WorldUnit to_world_unit() const;
};
}  // namespace yabe::core::unit
