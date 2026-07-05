#include "unit.hpp"

#include <cstdint>

namespace yabe::core::unit {
namespace {
constexpr float kPixelPerMeter = 2500.F;
}

Pixel WorldUnit::to_pixel() const {
  return Pixel{static_cast<int32_t>(this->value * kPixelPerMeter)};
}

WorldUnit Pixel::to_world_unit() const {
  return WorldUnit{static_cast<float>(this->value) / kPixelPerMeter};
}
}  // namespace yabe::core::unit
