#include "coordinate.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace yabe::common::coordinate {
CartesianCoord SphericalCoord::to_cartesian() const {
  return CartesianCoord({
      +this->radius * std::cos(this->phi) * std::sin(this->theta),
      +this->radius * std::sin(this->phi),
      -this->radius * std::cos(this->phi) * std::cos(this->theta),
  });
}

SphericalCoord CartesianCoord::to_spherical() const {
  return SphericalCoord{
      .radius = glm::length(this->pos),
      .theta  = std::atan2(this->pos.x, -this->pos.z),
      .phi    = std::asin(glm::normalize(this->pos).y),
  };
}
}  // namespace yabe::common::coordinate
