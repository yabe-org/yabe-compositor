#include "surface-node.hpp"

#include "common/geometry.hpp"
#include "core/scene/scene-node.hpp"
#include "core/surface.hpp"

namespace yabe::core::scene {
SurfaceNode::SurfaceNode(std::shared_ptr<core::Surface>&& surface)
    : SceneNode(Geometry(glm::vec3{0.F, 0.F, -kSurfaceDistance}))
    , surface_(std::move(surface)) {
  // scale (size) will be set by wl_surface.commit()
  this->geometry().scale().x = 0.F;
  this->geometry().scale().y = 0.F;
}

void SurfaceNode::draw_self(render::DrawRectFunc& draw_rect) {
  this->surface_->upload_texture_if_needed();
  draw_rect(this->geometry().model_mat(), this->surface_->texture());
  this->surface_->finish_rendering();
}

std::optional<common::intersection::IntersectInfo>
SurfaceNode::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction) {
  auto      geom_mat          = this->geometry().model_mat();
  glm::vec3 vert_left_bottom  = geom_mat * glm::vec4(-0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = geom_mat * glm::vec4(+0.5F, -0.5F, 0.F, 1.F);
  glm::vec3 vert_left_top     = geom_mat * glm::vec4(-0.5F, +0.5F, 0.F, 1.F);
  auto      result            = common::intersection::with_surface(
      origin, direction, vert_left_bottom, vert_right_bottom, vert_left_top);
  if (!result.has_value()) {
    return std::nullopt;
  }
  if (result->u < 0.F || result->u > 1.F) {
    return std::nullopt;
  }
  if (result->v < 0.F || result->v > 1.F) {
    return std::nullopt;
  }
  return common::intersection::IntersectInfo{
      .distance = result->distance,
      .pos{static_cast<float>(*this->surface_->texture_width()) * result->u,
           static_cast<float>(*this->surface_->texture_height()) *
              (1.F - result->v),
           0.F}
  };
}
}  // namespace yabe::core::scene
