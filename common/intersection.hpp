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

/// return distance if the vector is intersected with oriented bounding box
/// @param origin    The starting point of the vector
/// @param direction The direction of the vector
/// @param half_size The size of axis-aligned bounding box
std::optional<float> with_obb(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& half_size,
    const glm::mat4& model_mat);
}  // namespace yabe::common::intersection
