#include "intersection.hpp"

#include <glm/ext.hpp>
#include <optional>

namespace yabe::common::intersection {
// using Möller–Trumbore intersection algorithm
std::optional<SurfaceInfo> with_surface(const glm::vec3& o,
    const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1,
    const glm::vec3& v2) {
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
  const float     t   = glm::dot(q, e2) / det;
  if (t < 0) {
    return std::nullopt;
  }
  return SurfaceInfo{
      .distance = t,
      .u        = glm::dot(p, w) / det,
      .v        = glm::dot(q, d) / det,
  };
}
}  // namespace yabe::common::intersection
