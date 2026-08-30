#include "coordinate.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace yabe::common::coordinate {
CartesianCoord SphericalCoord::to_cartesian() const {
  return CartesianCoord({
      +this->radius * std::cos(this->polar) * std::sin(this->azimuth),
      +this->radius * std::sin(this->polar),
      -this->radius * std::cos(this->polar) * std::cos(this->azimuth),
  });
}

SphericalCoord CartesianCoord::to_spherical() const {
  return SphericalCoord{
      .radius  = glm::length(this->pos),
      .polar   = std::asin(glm::normalize(this->pos).y),
      .azimuth = std::atan2(this->pos.x, -this->pos.z),
  };
}
}  // namespace yabe::common::coordinate
