#include "intersection.hpp"

#include <cmath>
#include <glm/ext.hpp>
#include <limits>
#include <optional>
#include <utility>

namespace yabe::common::intersection {
// using Möller–Trumbore intersection algorithm
std::optional<SurfaceInfo> with_surface(const glm::vec3& o,
    const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1,
    const glm::vec3& v2) {
  if (glm::dot(dir, dir) <= std::numeric_limits<float>::epsilon()) {
    return std::nullopt;
  }
  const glm::vec3 d   = glm::normalize(dir);
  // Vectors that express the 'E'dges of the plane
  const glm::vec3 e1  = v1 - v0;
  const glm::vec3 e2  = v2 - v0;
  const glm::vec3 w   = o - v0;  // `T` in the original thesis
  // The any point P on the plane can be exprssed as:
  //   $P = vert0 + u*e0 + v*e1  (0 <= u, v <= 1)$
  // Note tht original algorithm limits u+v <= 1 because its target is triangle
  // If P is also expressed as follows, the vector and the plane is intersected
  //   $origin + t*direction (t >= 0)$
  const glm::vec3 p   = glm::cross(d, e2);
  const glm::vec3 q   = glm::cross(w, e1);
  const float     det = glm::dot(p, e1);
  if (std::abs(det) <= std::numeric_limits<float>::epsilon()) {
    return std::nullopt;
  }
  const float t = glm::dot(q, e2) / det;
  if (t < 0) {
    return std::nullopt;
  }
  return SurfaceInfo{
      .distance = t,
      .u        = glm::dot(p, w) / det,
      .v        = glm::dot(q, d) / det,
  };
}

std::optional<SurfaceInfo> with_surface(
    const glm::vec3& origin, const glm::vec3& direction, const glm::mat4& mat) {
  glm::vec3 vert_left_bottom  = mat * glm::vec4(-0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = mat * glm::vec4(+0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_left_top     = mat * glm::vec4(-0.5F, +0.5F, 0.F, 1.F);
  return common::intersection::with_surface(
      origin, direction, vert_left_bottom, vert_right_bottom, vert_left_top);
}

// based
// https://www.opengl-tutorial.org/jp/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access): glm::vec3[i] cannot be replaced
std::optional<float> with_obb(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& half_size,
    const glm::mat4& model_mat) {
  const glm::vec3 direction_norm = glm::normalize(direction);
  const glm::vec3 obb_world_pos  = glm::column(model_mat, 3);
  const glm::vec3 delta          = obb_world_pos - origin;

  float near = 0.F;
  float far  = std::numeric_limits<float>::max();
  for (int i = 0; i <= 2; ++i) {
    const glm::vec3 axis = glm::column(model_mat, i);
    const float     e    = glm::dot(axis, delta);
    const float     f    = glm::dot(direction_norm, axis);
    if (std::abs(f) > 0.001F) {
      float intersect_min = (e - half_size[i]) / f;
      float intersect_max = (e + half_size[i]) / f;
      if (intersect_min > intersect_max) {
        std::swap(intersect_min, intersect_max);
      }
      far  = std::min(far, intersect_max);
      near = std::max(near, intersect_min);
      if (far < near) {
        return std::nullopt;
      }
    } else if (-e - half_size[i] > 0.F || -e + half_size[i] < 0.F) {
      return std::nullopt;
    }
  }
  return near;
}
// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
}  // namespace yabe::common::intersection
