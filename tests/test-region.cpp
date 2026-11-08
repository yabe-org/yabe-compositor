#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "core/region.hpp"

namespace yabe::core {
namespace {
TEST(RegionAdd, AddsRectangleToEmptyRegion) {
  Region region;

  region.add(10, 20, 5, 4);

  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(20)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(23)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(9), common::unit::Pixel(20)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(20)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(19)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(24)));
}

TEST(RegionAdd, UnitesOverlappingRectangles) {
  Region region;

  region.add(10, 10, 10, 10);
  region.add(15, 15, 10, 10);

  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(10)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(17), common::unit::Pixel(17)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(24), common::unit::Pixel(24)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(20)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(20), common::unit::Pixel(14)));
}

TEST(RegionAdd, IgnoresRectanglesWithNonPositiveSize) {
  Region region;

  region.add(10, 10, 0, 10);
  region.add(10, 10, 10, 0);
  region.add(10, 10, -1, 10);
  region.add(10, 10, 10, -1);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(10)));
}

TEST(RegionContains, IncludesTopLeftAndExcludesBottomRightBoundaries) {
  Region region;
  region.add(10, 20, 5, 4);

  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(20)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(23)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(23)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(24)));
}

// +-----+
// | sub |
// |   +-+-----+
// +---+-+     |
//     | region|
//     +-------+
TEST(RegionSubtract, RemovesOverlapAtTopLeftCorner) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(5, 5, 10, 10);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(10)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(14)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(10)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(15)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(19)));
}

//         +-----+
//         | sub |
// +---+---+-+   |
// |       +-+---+
// | region  |
// +---------+
TEST(RegionSubtract, RemovesOverlapAtTopRightCorner) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(15, 5, 10, 10);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(10)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(14)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(10)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(15)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(19)));
}

//     +-------+
//     | region|
// +---+-+     |
// |   +-+-----+
// | sub |
// +-----+
TEST(RegionSubtract, RemovesOverlapAtBottomLeftCorner) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(5, 15, 10, 10);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(15)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(19)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(14)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(19)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(10)));
}

// +---------+
// | region  |
// |   +-----+-----+
// +---+-----+     |
//     |       sub |
//     +-----------+
TEST(RegionSubtract, RemovesOverlapAtBottomRightCorner) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(15, 15, 10, 10);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(15), common::unit::Pixel(15)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(19)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(14)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(19)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(10)));
}

// +-------------+
// | region      |
// |   +-----+   |
// |   | sub |   |
// |   +-----+   |
// +-------------+
TEST(RegionSubtract, RemovesRectangleFullyInsideRegion) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(12, 12, 6, 6);

  EXPECT_FALSE(
      region.contains(common::unit::Pixel(12), common::unit::Pixel(12)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(17), common::unit::Pixel(17)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(11), common::unit::Pixel(12)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(18), common::unit::Pixel(17)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(12), common::unit::Pixel(11)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(17), common::unit::Pixel(18)));
}

// +---------+   +-----+
// | region  |   | sub |
// +---------+   +-----+
TEST(RegionSubtract, PreservesRegionWhenRectangleIsOutside) {
  Region region;
  region.add(10, 10, 10, 10);

  region.subtract(20, 20, 5, 5);

  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(10)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(19), common::unit::Pixel(19)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(20), common::unit::Pixel(20)));
}

TEST(RegionAdd, ClampsRectangleWithMaximumSize) {
  Region region;

  region.add(10, 20, std::numeric_limits<int32_t>::max(),
      std::numeric_limits<int32_t>::max());

  EXPECT_TRUE(
      region.contains(common::unit::Pixel(10), common::unit::Pixel(20)));
  EXPECT_TRUE(region.contains(
      common::unit::Pixel(1 << 30), common::unit::Pixel(1 << 30)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(9), common::unit::Pixel(20)));
}

TEST(RegionAdd, UnitesRegions) {
  Region region;
  region.add(0, 0, 10, 10);
  Region other;
  other.add(5, 5, 10, 10);
  other.add(30, 30, 5, 5);

  region.add(other);

  EXPECT_TRUE(region.contains(common::unit::Pixel(0), common::unit::Pixel(0)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(14), common::unit::Pixel(14)));
  EXPECT_TRUE(
      region.contains(common::unit::Pixel(34), common::unit::Pixel(34)));
  EXPECT_FALSE(
      region.contains(common::unit::Pixel(12), common::unit::Pixel(2)));
}

TEST(RegionIntersect, ClipsRectanglesOutsideOfBounds) {
  Region region;
  region.add(0, 0, std::numeric_limits<int32_t>::max(),
      std::numeric_limits<int32_t>::max());
  region.add(-10, -10, 5, 5);

  region.intersect(0, 0, 100, 50);

  const auto boxes = region.boxes();
  ASSERT_EQ(boxes.size(), 1);
  EXPECT_EQ(boxes[0], (common::Box2D{
                          .x      = common::unit::Pixel(0),
                          .y      = common::unit::Pixel(0),
                          .width  = common::unit::Pixel(100),
                          .height = common::unit::Pixel(50),
                      }));
}

TEST(RegionIntersect, BecomesEmptyWhenNoRectangleIntersects) {
  Region region;
  region.add(200, 200, 10, 10);

  region.intersect(0, 0, 100, 100);

  EXPECT_TRUE(region.is_empty());
  EXPECT_TRUE(region.boxes().empty());
}

TEST(RegionBoxes, ReturnsNonOverlappingBoxes) {
  Region region;
  region.add(0, 0, 10, 10);
  region.add(5, 5, 10, 10);
  region.add(2, 2, 3, 3);

  int64_t area = 0;
  for (const auto& box : region.boxes()) {
    area += static_cast<int64_t>(*box.width) * *box.height;
  }

  // 10*10 + 10*10 - 5*5
  EXPECT_EQ(area, 175);
}

TEST(RegionClear, RemovesAllRectangles) {
  Region region;
  region.add(0, 0, 10, 10);

  region.clear();

  EXPECT_TRUE(region.is_empty());
  EXPECT_FALSE(region.contains(common::unit::Pixel(0), common::unit::Pixel(0)));
}
}  // namespace
}  // namespace yabe::core
