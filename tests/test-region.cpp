#include <gtest/gtest.h>

#include "core/region.hpp"

namespace yabe::core {
namespace {
TEST(RegionAdd, AddsRectangleToEmptyRegion) {
  Region region(nullptr);

  region.add(10, 20, 5, 4);

  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(20)));
  EXPECT_TRUE(region.contains(unit::Pixel(14), unit::Pixel(23)));
  EXPECT_FALSE(region.contains(unit::Pixel(9), unit::Pixel(20)));
  EXPECT_FALSE(region.contains(unit::Pixel(15), unit::Pixel(20)));
  EXPECT_FALSE(region.contains(unit::Pixel(10), unit::Pixel(19)));
  EXPECT_FALSE(region.contains(unit::Pixel(10), unit::Pixel(24)));
}

TEST(RegionAdd, UnitesOverlappingRectangles) {
  Region region(nullptr);

  region.add(10, 10, 10, 10);
  region.add(15, 15, 10, 10);

  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(10)));
  EXPECT_TRUE(region.contains(unit::Pixel(17), unit::Pixel(17)));
  EXPECT_TRUE(region.contains(unit::Pixel(24), unit::Pixel(24)));
  EXPECT_FALSE(region.contains(unit::Pixel(14), unit::Pixel(20)));
  EXPECT_FALSE(region.contains(unit::Pixel(20), unit::Pixel(14)));
}

TEST(RegionAdd, IgnoresRectanglesWithNonPositiveSize) {
  Region region(nullptr);

  region.add(10, 10, 0, 10);
  region.add(10, 10, 10, 0);
  region.add(10, 10, -1, 10);
  region.add(10, 10, 10, -1);

  EXPECT_FALSE(region.contains(unit::Pixel(10), unit::Pixel(10)));
}

TEST(RegionContains, IncludesTopLeftAndExcludesBottomRightBoundaries) {
  Region region(nullptr);
  region.add(10, 20, 5, 4);

  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(20)));
  EXPECT_TRUE(region.contains(unit::Pixel(14), unit::Pixel(23)));
  EXPECT_FALSE(region.contains(unit::Pixel(15), unit::Pixel(23)));
  EXPECT_FALSE(region.contains(unit::Pixel(14), unit::Pixel(24)));
}

// +-----+
// | sub |
// |   +-+-----+
// +---+-+     |
//     | region|
//     +-------+
TEST(RegionSubtract, RemovesOverlapAtTopLeftCorner) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(5, 5, 10, 10);

  EXPECT_FALSE(region.contains(unit::Pixel(10), unit::Pixel(10)));
  EXPECT_FALSE(region.contains(unit::Pixel(14), unit::Pixel(14)));
  EXPECT_TRUE(region.contains(unit::Pixel(15), unit::Pixel(10)));
  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(15)));
  EXPECT_TRUE(region.contains(unit::Pixel(19), unit::Pixel(19)));
}

//         +-----+
//         | sub |
// +---+---+-+   |
// |       +-+---+
// | region  |
// +---------+
TEST(RegionSubtract, RemovesOverlapAtTopRightCorner) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(15, 5, 10, 10);

  EXPECT_FALSE(region.contains(unit::Pixel(15), unit::Pixel(10)));
  EXPECT_FALSE(region.contains(unit::Pixel(19), unit::Pixel(14)));
  EXPECT_TRUE(region.contains(unit::Pixel(14), unit::Pixel(10)));
  EXPECT_TRUE(region.contains(unit::Pixel(19), unit::Pixel(15)));
  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(19)));
}

//     +-------+
//     | region|
// +---+-+     |
// |   +-+-----+
// | sub |
// +-----+
TEST(RegionSubtract, RemovesOverlapAtBottomLeftCorner) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(5, 15, 10, 10);

  EXPECT_FALSE(region.contains(unit::Pixel(10), unit::Pixel(15)));
  EXPECT_FALSE(region.contains(unit::Pixel(14), unit::Pixel(19)));
  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(14)));
  EXPECT_TRUE(region.contains(unit::Pixel(15), unit::Pixel(19)));
  EXPECT_TRUE(region.contains(unit::Pixel(19), unit::Pixel(10)));
}

// +---------+
// | region  |
// |   +-----+-----+
// +---+-----+     |
//     |       sub |
//     +-----------+
TEST(RegionSubtract, RemovesOverlapAtBottomRightCorner) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(15, 15, 10, 10);

  EXPECT_FALSE(region.contains(unit::Pixel(15), unit::Pixel(15)));
  EXPECT_FALSE(region.contains(unit::Pixel(19), unit::Pixel(19)));
  EXPECT_TRUE(region.contains(unit::Pixel(19), unit::Pixel(14)));
  EXPECT_TRUE(region.contains(unit::Pixel(14), unit::Pixel(19)));
  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(10)));
}

// +-------------+
// | region      |
// |   +-----+   |
// |   | sub |   |
// |   +-----+   |
// +-------------+
TEST(RegionSubtract, RemovesRectangleFullyInsideRegion) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(12, 12, 6, 6);

  EXPECT_FALSE(region.contains(unit::Pixel(12), unit::Pixel(12)));
  EXPECT_FALSE(region.contains(unit::Pixel(17), unit::Pixel(17)));
  EXPECT_TRUE(region.contains(unit::Pixel(11), unit::Pixel(12)));
  EXPECT_TRUE(region.contains(unit::Pixel(18), unit::Pixel(17)));
  EXPECT_TRUE(region.contains(unit::Pixel(12), unit::Pixel(11)));
  EXPECT_TRUE(region.contains(unit::Pixel(17), unit::Pixel(18)));
}

// +---------+   +-----+
// | region  |   | sub |
// +---------+   +-----+
TEST(RegionSubtract, PreservesRegionWhenRectangleIsOutside) {
  Region region(nullptr);
  region.add(10, 10, 10, 10);

  region.subtract(20, 20, 5, 5);

  EXPECT_TRUE(region.contains(unit::Pixel(10), unit::Pixel(10)));
  EXPECT_TRUE(region.contains(unit::Pixel(19), unit::Pixel(19)));
  EXPECT_FALSE(region.contains(unit::Pixel(20), unit::Pixel(20)));
}
}  // namespace
}  // namespace yabe::core
