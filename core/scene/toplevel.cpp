#include "toplevel.hpp"

#include <optional>

#include "common/intersection.hpp"

namespace yabe::core::scene {
Toplevel::Toplevel(std::weak_ptr<core::Surface>&& surface)
    : SceneNode(std::move(surface)), geometry_(glm::vec3(0.F, 0.F, -0.7F)) {
}
void Toplevel::move_absolute(const glm::vec3& pos) {
  this->geometry_.move_absolute(pos);
}
void Toplevel::move_relative(float polar, float azimuth) {
  this->geometry_.move_relative(azimuth, polar, 0.F);
}

void Toplevel::draw(render::DrawRectFunc& draw_rect) {
  const auto surface = this->surface_.lock();
  if (!surface) {
    return;
  }
  surface->upload_texture_if_needed();
  draw_rect(this->model_mat(), surface->texture());
  surface->finish_rendering();
}

glm::mat4 Toplevel::model_mat() {
  const auto surface = this->surface_.lock();
  if (!surface) {
    return glm::mat4(1.F);
  }
  glm::mat4  model_mat(1.F);
  const auto pos = this->geometry_.pos();

  const auto distance_from_y_axis = glm::length(glm::vec2(pos.x, pos.z));
  // if the distance from Y axis is zero, yaw can't be determined
  const auto yaw = distance_from_y_axis > 0.F ? glm::atan(-pos.x, -pos.z) : 0.F;
  const auto pitch = glm::atan(pos.y, distance_from_y_axis);
  auto rotation    = glm::rotate(glm::mat4(1.F), yaw, glm::vec3(0.F, 1.F, 0.F));
  rotation         = glm::rotate(rotation, pitch, glm::vec3(1.F, 0.F, 0.F));

  model_mat = glm::translate(model_mat, pos) * rotation *
              glm::scale(glm::mat4(1.F),
                  glm::vec3(*surface->texture_width().to_world_unit(),
                      *surface->texture_height().to_world_unit(), 0.F));
  return model_mat;
}

std::optional<common::intersection::IntersectInfo>
Toplevel::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction) {
  const auto surface = this->surface_.lock();
  if (!surface) {
    return std::nullopt;
  }
  auto      geom_mat          = this->model_mat();
  glm::vec3 vert_left_bottom  = geom_mat * glm::vec4(-0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = geom_mat * glm::vec4(+0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_left_top     = geom_mat * glm::vec4(-0.5F, +0.5F, 0.F, 1.F);
  auto      result            = common::intersection::with_surface(
      origin, direction, vert_left_bottom, vert_right_bottom, vert_left_top);
  if (!result.has_value()) {
    return std::nullopt;
  }
  return common::intersection::IntersectInfo{
      .distance = result->distance,
      .pos{static_cast<float>(*surface->texture_width()) * result->u,
           static_cast<float>(*surface->texture_height()) * (1.F - result->v),
           0.F}
  };
}

}  // namespace yabe::core::scene
