#pragma once

#include <cstdint>

namespace yabe::core::unit {
struct Pixel;
struct WorldUnit {
  float value;

  WorldUnit operator+(const WorldUnit& other) const {
    return WorldUnit{this->value + other.value};
  }
  WorldUnit operator-(const WorldUnit& other) const {
    return WorldUnit{this->value - other.value};
  }
  template <typename T>
  WorldUnit operator*(T scalar) const {
    return WorldUnit{this->value * scalar};
  }
  template <typename T>
  WorldUnit operator/(T scalar) const {
    return WorldUnit{this->value / scalar};
  }

  float operator*() const {
    return this->value;
  }
  [[nodiscard]] Pixel to_pixel() const;
};

struct Pixel {
  int32_t value;

  Pixel operator+(const Pixel& other) const {
    return Pixel{this->value + other.value};
  }
  Pixel operator-(const Pixel& other) const {
    return Pixel{this->value - other.value};
  }
  void operator+=(const Pixel& other) {
    this->value += other.value;
  }
  void operator-=(const Pixel& other) {
    this->value -= other.value;
  }
  template <typename T>
  Pixel operator*(T scalar) const {
    return Pixel{static_cast<int32_t>(this->value * scalar)};
  }
  template <typename T>
  Pixel operator/(T scalar) const {
    return Pixel{static_cast<int32_t>(this->value / scalar)};
  }

  int32_t operator*() const {
    return this->value;
  }
  [[nodiscard]] WorldUnit to_world_unit() const;
};
}  // namespace yabe::core::unit
