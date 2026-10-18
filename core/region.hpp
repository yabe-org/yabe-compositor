#pragma once

#include <wayland-server.h>

#include <vector>

#include "common/box.hpp"
#include "common/unit.hpp"

namespace yabe::core {
class Region {
 public:
  explicit Region(wl_resource* resource = nullptr);
  ~Region() = default;

  Region(const Region&)            = default;
  Region(Region&&)                 = default;
  Region& operator=(const Region&) = default;
  Region& operator=(Region&&)      = default;

  void add(int32_t x, int32_t y, int32_t width, int32_t height);
  void add(const Region& other);
  void subtract(int32_t x, int32_t y, int32_t width, int32_t height);
  /// remove the area outside of the given rect
  void intersect(int32_t x, int32_t y, int32_t width, int32_t height);
  void clear();
  [[nodiscard]] bool contains(
      common::unit::Pixel x, common::unit::Pixel y) const;
  [[nodiscard]] bool is_empty() const {
    return this->rects_.empty();
  }
  /// non-overlapping rects that make up this region
  [[nodiscard]] std::vector<common::Box2D> boxes() const;

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

    /// the end is clamped to INT32_MAX
    /// because clients may specify INT32_MAX as width or height
    static Rect from(int32_t x, int32_t y, int32_t width, int32_t height);
  };
  void add(const Rect& rect);

  /// all rects are non-overlapping
  std::vector<Rect> rects_;

  wl_resource* resource_;
};
}  // namespace yabe::core
