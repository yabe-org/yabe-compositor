#include "surface-node.hpp"

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "common/intersection.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"
#include "core/surface.hpp"

namespace yabe::core::scene {
SurfaceNode::SurfaceNode(std::weak_ptr<core::Surface>&& surface)
    : SceneNode(Geometry(glm::vec3{0.F, 0.F, -kSurfaceDistance}))
    , surface_(std::move(surface)) {
  // scale (size) will be set by wl_surface.commit()
  this->geometry().scale().x = 0.F;
  this->geometry().scale().y = 0.F;
}

void SurfaceNode::draw_self(render::DrawRectFunc& draw_rect) {
  this->surface()->upload_texture_if_needed();
  draw_rect(this->geometry().model_mat(), this->surface()->texture());
  this->surface()->finish_rendering();
}

std::optional<SceneIntersection> SurfaceNode::check_intersection_self(
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
  const auto x =
      static_cast<float>(*this->surface()->texture_width()) * result->u;
  const auto y = static_cast<float>(*this->surface()->texture_height()) *
                 (1.F - result->v);
  const common::intersection::IntersectInfo info{
      .distance = result->distance,
      .pos      = {x, y, 0.F},
  };
  return SceneIntersection{
      .node           = this->shared_from_this(),
      .intersect_info = info,
  };
}
}  // namespace yabe::core::scene
