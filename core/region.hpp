#pragma once

#include <vector>

#include "common/unit.hpp"
#include "wayland-server.h"

namespace yabe::core {
class Region {
 public:
  explicit Region(wl_resource* resource);
  ~Region() = default;

  Region(const Region&)            = default;
  Region(Region&&)                 = default;
  Region& operator=(const Region&) = default;
  Region& operator=(Region&&)      = default;

  void add(int32_t x, int32_t y, int32_t width, int32_t height);
  void subtract(int32_t x, int32_t y, int32_t width, int32_t height);
  [[nodiscard]] bool contains(
      common::unit::Pixel x, common::unit::Pixel y) const;

 private:
  struct Rect {
    /// all positions are top-left originated position
    common::unit::Pixel begin_x{0};
    common::unit::Pixel begin_y{0};
    common::unit::Pixel end_x{0};
    common::unit::Pixel end_y{0};

    [[nodiscard]] bool              is_empty() const;
    [[nodiscard]] bool              intersects(const Rect& other) const;
    [[nodiscard]] std::vector<Rect> subtract(const Rect& other) const;
  };
  /// all rects are non-overlapping
  std::vector<Rect> rects_;

  wl_resource* resource_;
};
}  // namespace yabe::core
