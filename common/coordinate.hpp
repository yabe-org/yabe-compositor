#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace yabe::common::coordinate {
/*
         +Y  -Z (direction that the user initially looks at)
          ^  /
          | /
          |/
----------------*----> +X
         /|
        / |
       /  |
     +Z
Let asterisk (*) pos is [x, y, z], its values are:
  - x = +radius * cos(polar) * sin(azimuthal)
  - y = +radius * sin(polar)
  - z = -radius * cos(polar) * cos(azimuthal)
*/
struct CartesianCoord;
struct SphericalCoord {
  float radius;
  float theta;  // azimuthal angle, in radians
  float phi;    // polar angle, in radians

  [[nodiscard]] CartesianCoord to_cartesian() const;
};
struct CartesianCoord {
  glm::vec3 pos;
  CartesianCoord() : pos(0.F, 0.F, 0.F) {
  }
  explicit CartesianCoord(const glm::vec3& pos) : pos(pos) {
  }
  [[nodiscard]] SphericalCoord to_spherical() const;
};
}  // namespace yabe::common::coordinate
