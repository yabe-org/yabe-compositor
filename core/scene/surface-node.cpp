#include "surface-node.hpp"

#include "common/geometry.hpp"
#include "common/intersection.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::core::scene {
SurfaceNode::SurfaceNode(std::weak_ptr<core::Surface>&& surface, Anchor anchor)
    : surface_(std::move(surface)), anchor_(anchor) {
  // scale (size) will be set by wl_surface.commit()
  this->geometry().scale().x = 0.F;
  this->geometry().scale().y = 0.F;
}

glm::mat4 SurfaceNode::resolve_world_transform(
    const SceneNode* parent, const glm::mat4& parent_world_transform) const {
  if (this->anchor_ != Anchor::TOP_LEFT) {
    return SceneNode::resolve_world_transform(parent, parent_world_transform);
  }
  const auto* parent_surface = dynamic_cast<const SurfaceNode*>(parent);
  if (!parent_surface) {
    return SceneNode::resolve_world_transform(parent, parent_world_transform);
  }

  const auto& parent_scale              = parent_surface->geometry().scale();
  const auto& scale                     = this->geometry().scale();
  const auto  parent_center_to_top_left = glm::translate(glm::mat4(1.F),
      glm::vec3{-parent_scale.x / 2.F, parent_scale.y / 2.F, 0.F});
  const auto  top_left_to_center        = glm::translate(
      glm::mat4(1.F), glm::vec3{scale.x / 2.F, -scale.y / 2.F, 0.F});
  return parent_world_transform * parent_center_to_top_left *
         this->geometry().transform_mat() * top_left_to_center;
}

glm::mat4 SurfaceNode::model_mat() const {
  return this->world_transform() *
         glm::scale(glm::mat4(1.F), this->geometry().scale());
}

void SurfaceNode::prepare_for_draw() {
  this->surface()->upload_texture_if_needed();
}

void SurfaceNode::draw_self(
    const glm::mat4& vp_mat, const glm::mat4& world_transform) {
  const auto model_mat = vp_mat * world_transform *
                         glm::scale(glm::mat4(1.F), this->geometry().scale());
  server::get().rect_renderer.render(model_mat, this->surface()->texture());
  this->surface()->finish_rendering();
}

std::optional<SceneIntersection> SurfaceNode::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction,
    const glm::mat4& world_transform) {
  const auto geom_mat =
      world_transform * glm::scale(glm::mat4(1.F), this->geometry().scale());
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
  const auto        y = static_cast<float>(*this->surface()->texture_height()) *
                        (1.F - result->v);
  SceneIntersection intersection{
      .node           = this->shared_from_this(),
      .intersect_info = {.distance = result->distance,
                         .pos                     = glm::vec3{x, y, 0.F}},
  };
  const auto region = this->surface()->input_region();
  if (!region.has_value()) {
    return intersection;
  }
  if (region.value().contains(common::unit::Pixel(static_cast<int32_t>(x)),
          common::unit::Pixel(static_cast<int32_t>(y)))) {
    return intersection;
  }
  return std::nullopt;
}
}  // namespace yabe::core::scene
