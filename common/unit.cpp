#include "unit.hpp"

#include <cstdint>

#include "core/server.hpp"

namespace yabe::core::unit {
Pixel WorldUnit::to_pixel() const {
  return Pixel{static_cast<int32_t>(
      this->value * server::get().setting.config.get<"pixels_per_meter">())};
}

WorldUnit Pixel::to_world_unit() const {
  return WorldUnit{static_cast<float>(
      static_cast<double>(this->value) /
      server::get().setting.config.get<"pixels_per_meter">())};
}
}  // namespace yabe::core::unit
