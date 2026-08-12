#include <gtest/gtest.h>

#include "common/intersection.hpp"

namespace yabe::common::intersection {
namespace {
TEST(IntersectionWithSurface,
    ReturnsZeroCoordinatesWhenRayIntersectsWithSurfaceTopLeftEdge) {
  const auto result = with_surface(glm::vec3{0.F, 0.F, 0.F},
      glm::vec3{0.F, +0.F, -1.F}, glm::vec3{0.F, -1.F, -1.F},
      glm::vec3{1.F, -1.F, -1.F}, glm::vec3{0.F, +0.F, -1.F});

  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->distance, 1.F);
  EXPECT_FLOAT_EQ(result->u, 0.F);
  EXPECT_FLOAT_EQ(result->v, 1.F);
}

constexpr glm::vec3 kLeftBottom{0.F, 0.F, 0.F};
constexpr glm::vec3 kRightBottom{4.F, 0.F, 0.F};
constexpr glm::vec3 kLeftTop{0.F, 2.F, 0.F};

TEST(IntersectionWithSurface, ReturnsDistanceAndSurfaceCoordinates) {
  const auto result = with_surface(glm::vec3{1.F, 0.5F, 3.F},
      glm::vec3{0.F, 0.F, -1.F}, kLeftBottom, kRightBottom, kLeftTop);

  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->distance, 3.F);
  EXPECT_FLOAT_EQ(result->u, 0.25F);
  EXPECT_FLOAT_EQ(result->v, 0.25F);
}

TEST(IntersectionWithSurface, NormalizesDirectionBeforeCalculatingDistance) {
  const auto result = with_surface(glm::vec3{1.F, 0.5F, 6.F},
      glm::vec3{0.F, 0.F, -2.F}, kLeftBottom, kRightBottom, kLeftTop);

  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->distance, 6.F);
  EXPECT_FLOAT_EQ(result->u, 0.25F);
  EXPECT_FLOAT_EQ(result->v, 0.25F);
}

TEST(IntersectionWithSurface, ReturnsCoordinatesOutsideSurfaceBounds) {
  const auto result = with_surface(glm::vec3{5.F, 3.F, 1.F},
      glm::vec3{0.F, 0.F, -1.F}, kLeftBottom, kRightBottom, kLeftTop);

  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result->distance, 1.F);
  EXPECT_FLOAT_EQ(result->u, 1.25F);
  EXPECT_FLOAT_EQ(result->v, 1.5F);
}

TEST(IntersectionWithSurface, ReturnsNoIntersectionBehindRayOrigin) {
  const auto result = with_surface(glm::vec3{1.F, 0.5F, 1.F},
      glm::vec3{0.F, 0.F, 1.F}, kLeftBottom, kRightBottom, kLeftTop);

  EXPECT_FALSE(result.has_value());
}

TEST(IntersectionWithSurface, ReturnsNoIntersectionForParallelRay) {
  const auto result = with_surface(glm::vec3{1.F, 0.5F, 1.F},
      glm::vec3{1.F, 0.F, 0.F}, kLeftBottom, kRightBottom, kLeftTop);

  EXPECT_FALSE(result.has_value());
}

TEST(IntersectionWithSurface, ReturnsNoIntersectionForZeroDirection) {
  const auto result = with_surface(glm::vec3{1.F, 0.5F, 1.F},
      glm::vec3{0.F, 0.F, 0.F}, kLeftBottom, kRightBottom, kLeftTop);

  EXPECT_FALSE(result.has_value());
}
}  // namespace
}  // namespace yabe::common::intersection
