#include "toplevel-group-node.hpp"

#include <wayland-server-protocol.h>

#include <algorithm>
#include <core/server.hpp>
#include <cstdint>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <ranges>

#include "common/coordinate.hpp"
#include "common/unit.hpp"
#include "core/scene/decoration-node.hpp"
#include "core/scene/scene.hpp"

namespace yabe::core::scene {
namespace {
constexpr std::array<uint8_t, 4> kTextureColor{
    0x79, 0xc3, 0x98, 0xFF};  // ARGB, little endian
}  // namespace
void ToplevelGroupNode::init(std::list<std::shared_ptr<SceneNode>>& nodes) {
  this->decoration_ = std::make_shared<DecorationNode>();
  this->add_child(this->decoration_);
  for (auto& n : nodes) {
    this->add_child(n);
  }

  float                min_polar   = std::numeric_limits<float>::max();
  float                max_polar   = std::numeric_limits<float>::lowest();
  float                min_azimuth = std::numeric_limits<float>::max();
  float                max_azimuth = std::numeric_limits<float>::lowest();
  std::optional<float> base_azimuth;
  for (const auto& toplevel : this->toplevels()) {
    const auto pos   = toplevel->geometry().pos();
    auto       coord = common::coordinate::CartesianCoord(pos).to_spherical();
    if (base_azimuth) {
      coord.azimuth =
          *base_azimuth +  //
          static_cast<float>(std::remainder(
              coord.azimuth - *base_azimuth, 2.F * std::numbers::pi));
    } else {
      base_azimuth = coord.azimuth;
    }
    min_polar   = std::min(min_polar, coord.polar);
    max_polar   = std::max(max_polar, coord.polar);
    min_azimuth = std::min(min_azimuth, coord.azimuth);
    max_azimuth = std::max(max_azimuth, coord.azimuth);
  }
  this->move_absolute(common::coordinate::SphericalCoord{
      .radius  = kSceneNodeDistance,
      .polar   = (min_polar + max_polar) / 2.F,
      .azimuth = (min_azimuth + max_azimuth) / 2.F,
  }
          .to_cartesian()
          .pos);

  this->reconfigure();
}

void ToplevelGroupNode::reconfigure() {
  float width      = 0.F;
  float max_height = -1.F;
  for (const auto& toplevel : this->toplevels()) {
    width += toplevel->geometry().scale().x;
    max_height = std::max(max_height, toplevel->geometry().scale().y);
  }
  const auto padding = static_cast<float>(
      server::get().setting.config.get<"surface_grouping", "padding">());
  const auto spacing = static_cast<float>(
      server::get().setting.config.get<"surface_grouping", "window_spacing">());
  width +=
      (static_cast<float>(spacing) *
          static_cast<float>(std::ranges::distance(this->toplevels()) - 1)) +
      (2 * padding);
  this->geometry().scale() = glm::vec3{
      width,
      max_height + (2 * padding),
      1.F,
  };

  // arrange children like a flexbox
  glm::vec3 next_surface_pos{
      -(width / 2.F) + padding,
      0.F,
      0.F,
  };
  for (const auto& toplevel : this->toplevels()) {
    next_surface_pos.x += toplevel->geometry().scale().x / 2.F;
    toplevel->move_absolute(next_surface_pos);
    toplevel->geometry().rotation() = glm::quat();
    next_surface_pos.x += (toplevel->geometry().scale().x / 2.F) + spacing;
  }

  this->look_at_camera();
}

void ToplevelGroupNode::prepare_for_draw() {
  if (!this->is_texture_uploaded_) {
    const common::unit::Pixel size{1};
    this->bg_texture_.upload(  // NOLINTNEXTLINE(google-readability-casting)
        (void*)(&kTextureColor), WL_SHM_FORMAT_ARGB8888, size, size, 1);
    this->is_texture_uploaded_ = true;
  }
}
void ToplevelGroupNode::draw_self(const glm::mat4& proj_mat,
    const glm::mat4& view_mat, const glm::mat4& world_transform) {
  const auto model_mat = proj_mat * view_mat * world_transform *
                         glm::scale(glm::mat4(1.F), this->geometry().scale());
  server::get().rect_renderer.render(model_mat, this->bg_texture_);
}

std::optional<SceneIntersection> ToplevelGroupNode::check_intersection_self(
    const glm::vec3& origin, const glm::vec3& direction,
    const glm::mat4& world_transform) {
  const auto geom_mat =
      world_transform * glm::scale(glm::mat4(1.F), this->geometry().scale());
  auto result = common::intersection::with_surface(origin, direction, geom_mat);
  if (!result.has_value()) {
    return std::nullopt;
  }
  if (result->u < 0.F || result->u > 1.F) {
    return std::nullopt;
  }
  if (result->v < 0.F || result->v > 1.F) {
    return std::nullopt;
  }
  const auto x = this->geometry().scale().x * result->u;
  const auto y = this->geometry().scale().y * (1.F - result->v);
  return SceneIntersection{
      .node           = this->shared_from_this(),
      .intersect_info = {.distance = result->distance,
                         .pos                     = glm::vec3{x, y, 0.F}},
  };
}

void ToplevelGroupNode::handle_child_erased() {
  if (std::ranges::empty(this->toplevels())) {
    server::get().enqueue_wayland_thread_task(
        [node = this->shared_from_this()] {
          core::server::get().workspace.current_scene().erase(node);
        });
  } else {
    this->reconfigure();
  }
}
void ToplevelGroupNode::handle_child_resized() {
  this->reconfigure();
}
bool ToplevelGroupNode::handle_child_moved(
    const std::shared_ptr<SceneNode>& child, float dx, float dy) {
  const auto threshold =
      server::get()
          .setting.config.get<"surface_grouping", "detach_threshold">();
  // usually dx&dy are too short and less than 1, so multiply their length
  // not to force users to set too small value for it
  if (((dx * dx) + (dy * dy)) * 10'000 > threshold) {
    child->detach_from_parent();
  }
  // do not move child
  return true;
}
bool ToplevelGroupNode::try_raise_child_to_front(
    const std::shared_ptr<SceneNode>& child) {
  if (std::ranges::find(this->children_, child) != this->children_.end()) {
    // do not raise to keep order of children
    return true;
  }
  return SceneNode::try_raise_child_to_front(child);
}
}  // namespace yabe::core::scene
