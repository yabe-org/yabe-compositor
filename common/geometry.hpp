#pragma once

#include <glm/glm.hpp>

#include "coordinate.hpp"

namespace yabe::core {
// TODO: size and rotation
class Geometry {
 public:
  explicit Geometry(const glm::vec3& pos) : cartesian_(pos) {
  }
  [[nodiscard]] glm::vec3& pos() {
    return this->cartesian_.pos;
  }

  void move_absolute(const glm::vec3& new_cartesian);
  void move_absolute(float new_theta, float new_phi, float new_radius);

  void move_relative(const glm::vec3& d_cartesian);
  void move_relative(float d_theta, float d_phi, float d_radius);

 private:
  coordinate::CartesianCoord cartesian_;
};
}  // namespace yabe::core
