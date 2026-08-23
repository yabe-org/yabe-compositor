#pragma once

#include "unit.hpp"

namespace yabe::common {
struct Box2D {
  common::unit::Pixel x{0};
  common::unit::Pixel y{0};
  common::unit::Pixel width{0};
  common::unit::Pixel height{0};

  auto operator<=>(const Box2D&) const = default;
};
}  // namespace yabe::common
