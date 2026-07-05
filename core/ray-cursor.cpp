#include "ray-cursor.hpp"

namespace yabe::core {
void RayCursor::move_relative(float polar, float azimuth) {
  this->azimuth_ += azimuth;

  constexpr float kPolarMin = std::numbers::pi / 8.F;
  constexpr float kPolarMax = std::numbers::pi - kPolarMin;
  this->polar_ = std::clamp(this->polar_ + polar, kPolarMin, kPolarMax);
}
}  // namespace yabe::core
