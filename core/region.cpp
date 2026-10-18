#include "region.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>

namespace yabe::core {
Region::Region(wl_resource* resource) : resource_(resource) {
}

void Region::add(int32_t x, int32_t y, int32_t width, int32_t height) {
  this->add(Rect::from(x, y, width, height));
}
void Region::add(const Region& other) {
  for (const auto& rect : other.rects_) {
    this->add(rect);
  }
}
void Region::add(const Rect& rect) {
  if (rect.is_empty()) {
    return;
  }

  std::vector<Rect> remaining_new_rects{rect};
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
  const auto sub_rect = Rect::from(x, y, width, height);
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
// intersect() and boxes() allocate a new std::vector on every call.
// This is acceptable because they are only called right before uploading a
// newly committed buffer (not every frame), and the region has almost
// always been updated since the last call, so there is nothing to cache.
void Region::intersect(int32_t x, int32_t y, int32_t width, int32_t height) {
  const auto        clip_rect = Rect::from(x, y, width, height);
  std::vector<Rect> new_rects;
  for (const auto& rect : this->rects_) {
    if (!rect.intersects(clip_rect)) {
      continue;
    }
    new_rects.push_back(Rect{
        .begin_x = std::max(rect.begin_x, clip_rect.begin_x),
        .begin_y = std::max(rect.begin_y, clip_rect.begin_y),
        .end_x   = std::min(rect.end_x, clip_rect.end_x),
        .end_y   = std::min(rect.end_y, clip_rect.end_y),
    });
  }
  this->rects_ = std::move(new_rects);
}
void Region::clear() {
  this->rects_.clear();
}
bool Region::contains(common::unit::Pixel x, common::unit::Pixel y) const {
  return std::ranges::any_of(
      this->rects_.begin(), this->rects_.end(), [x, y](const Rect& rect) {
        return rect.begin_x <= x && x < rect.end_x &&  //
               rect.begin_y <= y && y < rect.end_y;
      });
}

std::vector<common::Box2D> Region::boxes() const {
  std::vector<common::Box2D> result;
  result.reserve(this->rects_.size());
  for (const auto& rect : this->rects_) {
    result.push_back(common::Box2D{
        .x      = rect.begin_x,
        .y      = rect.begin_y,
        .width  = rect.end_x - rect.begin_x,
        .height = rect.end_y - rect.begin_y,
    });
  }
  return result;
}

Region::Rect Region::Rect::from(
    int32_t x, int32_t y, int32_t width, int32_t height) {
  const auto clamped_end = [](int32_t begin, int32_t length) {
    return static_cast<int32_t>(
        std::min<int64_t>(static_cast<int64_t>(begin) + length,
            std::numeric_limits<int32_t>::max()));
  };
  return Rect{
      .begin_x = common::unit::Pixel(x),
      .begin_y = common::unit::Pixel(y),
      .end_x   = common::unit::Pixel(clamped_end(x, width)),
      .end_y   = common::unit::Pixel(clamped_end(y, height)),
  };
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
