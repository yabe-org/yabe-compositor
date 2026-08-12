#include "region.hpp"

#include <algorithm>
#include <iterator>

namespace yabe::core {
Region::Region(wl_resource* resource) : resource_(resource) {
}

void Region::add(int32_t x, int32_t y, int32_t width, int32_t height) {
  Rect new_rect{
      .begin_x = unit::Pixel(x),
      .begin_y = unit::Pixel(y),
      .end_x   = unit::Pixel(x + width),
      .end_y   = unit::Pixel(y + height),
  };
  if (new_rect.is_empty()) {
    return;
  }

  std::vector<Rect> remaining_new_rects{new_rect};
  for (const auto& existing_rect : this->rects_) {
    std::vector<Rect> next;
    for (const auto& new_rect : remaining_new_rects) {
      // to avoid overlap, subtract the new rect from existing rects first
      const auto subtracted = new_rect.subtract(existing_rect);
      next.insert(next.end(), subtracted.begin(), subtracted.end());
    }
    if (next.empty()) {
      // new_rect is fully covered by existing rects
      return;
    }
    remaining_new_rects = std::move(next);
  }
  // all rects in remaining_new_rects are non-overlapping with existing rects
  this->rects_.insert(this->rects_.end(),
      std::make_move_iterator(remaining_new_rects.begin()),
      std::make_move_iterator(remaining_new_rects.end()));
}

void Region::subtract(int32_t x, int32_t y, int32_t width, int32_t height) {
  Rect sub_rect{
      .begin_x = unit::Pixel(x),
      .begin_y = unit::Pixel(y),
      .end_x   = unit::Pixel(x + width),
      .end_y   = unit::Pixel(y + height),
  };
  if (sub_rect.is_empty()) {
    return;
  }
  std::vector<Rect> new_rects;
  for (const auto& rect : this->rects_) {
    const auto remaining_rects = rect.subtract(sub_rect);
    new_rects.insert(
        new_rects.end(), remaining_rects.begin(), remaining_rects.end());
  }
  this->rects_ = std::move(new_rects);
}
bool Region::contains(unit::Pixel x, unit::Pixel y) const {
  return std::ranges::any_of(
      this->rects_.begin(), this->rects_.end(), [x, y](const Rect& rect) {
        return rect.begin_x <= x && x < rect.end_x &&  //
               rect.begin_y <= y && y < rect.end_y;
      });
}

bool Region::Rect::is_empty() const {
  return this->begin_x >= this->end_x || this->begin_y >= this->end_y;
}
bool Region::Rect::intersects(const Rect& other) const {
  return !(this->end_x <= other.begin_x || this->begin_x >= other.end_x ||
           this->end_y <= other.begin_y || this->begin_y >= other.end_y);
}
std::vector<Region::Rect> Region::Rect::subtract(const Rect& other) const {
  if (other.is_empty() || !this->intersects(other)) {
    return {*this};
  }
  /*
    Assume that:
      +-----------------+      +-----------------+
      |       TOP       |      |       TOP       |
      +---B---------+---+      +---B-------------+
      |   |         |   |      |   |             |
      | L | removed | R |  or  | L |             |
      |   |         |   |      |   |   removed   |
      +---+---------E---+      |   |             |
      |      BOTTOM     |      |   |             |
      +-----------------+      +---+-------------E
  */
  // B
  const auto        begin_x = std::max(this->begin_x, other.begin_x);
  const auto        begin_y = std::max(this->begin_y, other.begin_y);
  // E
  const auto        end_x   = std::min(this->end_x, other.end_x);
  const auto        end_y   = std::min(this->end_y, other.end_y);
  std::vector<Rect> result;
  // top
  if (this->begin_y < begin_y) {
    result.push_back(Rect{
        .begin_x = this->begin_x,
        .begin_y = this->begin_y,
        .end_x   = this->end_x,
        .end_y   = begin_y,
    });
  }
  // bottom
  if (this->end_y > end_y) {
    result.push_back(Rect{
        .begin_x = this->begin_x,
        .begin_y = end_y,
        .end_x   = this->end_x,
        .end_y   = this->end_y,
    });
  }
  // left
  if (this->begin_x < begin_x) {
    result.push_back(Rect{
        .begin_x = this->begin_x,
        .begin_y = begin_y,
        .end_x   = begin_x,
        .end_y   = end_y,
    });
  }
  // right
  if (this->end_x > end_x) {
    result.push_back(Rect{
        .begin_x = end_x,
        .begin_y = begin_y,
        .end_x   = this->end_x,
        .end_y   = end_y,
    });
  }
  return result;
}
}  // namespace yabe::core
