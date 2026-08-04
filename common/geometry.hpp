#pragma once

#include <glm/ext.hpp>
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
  [[nodiscard]] glm::quat& rotation() {
    return this->rotation_;
  }
  [[nodiscard]] glm::vec3& scale() {
    return this->scale_;
  }
  [[nodiscard]] glm::mat4 model_mat();

  void move_absolute(const glm::vec3& new_cartesian);
  void move_absolute(float new_theta, float new_phi, float new_radius);

  void move_relative(const glm::vec3& d_cartesian);
  void move_relative(float d_theta, float d_phi, float d_radius);

  void set_rotation(const glm::quat& new_rotation);

 private:
  coordinate::CartesianCoord cartesian_;
  glm::quat                  rotation_ = glm::quat(1.F, 0.F, 0.F, 0.F);
  glm::vec3                  scale_    = glm::vec3(1.F, 1.F, 1.F);
};
}  // namespace yabe::core
