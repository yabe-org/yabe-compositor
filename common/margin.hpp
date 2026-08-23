#pragma once

#include "common/unit.hpp"

namespace yabe::common {
struct Margin {
  unit::Pixel top{0};
  unit::Pixel bottom{0};
  unit::Pixel left{0};
  unit::Pixel right{0};

  void set_x(unit::Pixel x) {
    this->left  = x;
    this->right = x;
  }
  void set_y(unit::Pixel y) {
    this->left  = y;
    this->right = y;
  }
};
}  // namespace yabe::common
