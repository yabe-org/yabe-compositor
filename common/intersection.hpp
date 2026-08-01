#pragma once

#include <glm/ext.hpp>
#include <optional>

namespace yabe::common::intersection {
struct IntersectInfo {
  float     distance;
  glm::vec3 pos;
};
struct SurfaceInfo {
  float distance;
  float u, v;
};
/// @return intersection info if the vector is intersected with plane
/// @param origin    The starting point of the vector
/// @param direction The direction of the vector
std::optional<SurfaceInfo> with_surface(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& vert_left_bottom,
    const glm::vec3& vert_right_bottom, const glm::vec3& vert_left_top);
}  // namespace yabe::common::intersection
