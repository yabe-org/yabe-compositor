#pragma once

#include <wayland-util.h>
namespace yabe::common {
/// screen-local coordinates
struct ScreenPos {
  ScreenPos(double x, double y) : x(x), y(y) {
  }
  double x, y;
};

/// surface-local coordinates
struct SurfacePos {
  SurfacePos(double x, double y) : x(x), y(y) {
  }
  double x, y;
};
/// surface-local coordinates but using wl_fixed_t
struct WlFixedSurfacePos {
  explicit WlFixedSurfacePos(SurfacePos pos)
      : x(wl_fixed_from_double(pos.x)), y(wl_fixed_from_double(pos.y)) {
  }
  wl_fixed_t x, y;
};
}  // namespace yabe::common
