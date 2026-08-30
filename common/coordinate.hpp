#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace yabe::common::coordinate {
/*
         +Y   -Z (direction that the user initially looks at)
          ^   /
          |  o
          x /
          |/
----------------*----> +X
         /|
        / |
       /  |
     +Z
Let asterisk (*) pos is [x, y, z], its values are:
  - x = +radius * cos(polar) * sin(azimuth)
  - y = +radius * sin(polar)
  - z = -radius * cos(polar) * cos(azimuth)
  => [polar = 0, azimuth = pi/2]

Identically,
  - `o` pos [0, 0, 1] : [polar = 0,    azimuth = 0]
  - `x` pos [0, 1, 0] : [polar = pi/2, azimuth = 0]
*/
struct CartesianCoord;
struct SphericalCoord {
  float radius;
  float polar;    // in radians
  float azimuth;  // x-z plane, in radians

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
