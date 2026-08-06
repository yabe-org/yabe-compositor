#include "geometry.hpp"

namespace yabe::core {
glm::mat4 Geometry::transform_mat() const {
  glm::mat4 model_mat(1.F);
  model_mat = glm::translate(model_mat, this->cartesian_.pos) *
              glm::mat4_cast(this->rotation_);
  return model_mat;
}

glm::mat4 Geometry::model_mat() const {
  return this->transform_mat() * glm::scale(glm::mat4(1.F), this->scale_);
}

void Geometry::move_absolute(const glm::vec3& new_cartesian) {
  this->cartesian_.pos = new_cartesian;
}
void Geometry::move_absolute(float new_theta, float new_phi, float new_radius) {
  const coordinate::SphericalCoord spherical{
      .radius = new_radius,
      .theta  = new_theta,
      .phi    = new_phi,
  };
  this->cartesian_ = spherical.to_cartesian();
}

void Geometry::move_relative(const glm::vec3& d_cartesian) {
  this->cartesian_.pos += d_cartesian;
}
void Geometry::move_relative(float d_theta, float d_phi, float d_radius) {
  auto spherical = this->cartesian_.to_spherical();
  spherical.theta += d_theta;
  spherical.phi += d_phi;
  spherical.radius += d_radius;
  this->cartesian_ = spherical.to_cartesian();
}

}  // namespace yabe::core
