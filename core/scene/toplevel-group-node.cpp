#include "toplevel-group-node.hpp"

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

#include <core/server.hpp>
#include <cstdint>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <numbers>
#include <optional>

#include "common/coordinate.hpp"
#include "common/unit.hpp"
#include "core/server-seat/pointer-grab/move.hpp"

namespace yabe::core::scene {
namespace {
constexpr std::array<uint8_t, 4> kTextureColor{
    0x79, 0xc3, 0x98, 0xFF};  // ARGB, little endian
}  // namespace
void ToplevelGroupNode::init(std::list<std::shared_ptr<SceneNode>>& nodes) {
  for (auto& n : nodes) {
    this->add_child(n);
  }

  float                min_polar   = std::numeric_limits<float>::max();
  float                max_polar   = std::numeric_limits<float>::lowest();
  float                min_azimuth = std::numeric_limits<float>::max();
  float                max_azimuth = std::numeric_limits<float>::lowest();
  std::optional<float> base_azimuth;
  for (const auto& child : this->children_) {
    const auto pos   = child->geometry().pos();
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
  for (const auto& child : this->children_) {
    width += child->geometry().scale().x;
    max_height = std::max(max_height, child->geometry().scale().y);
  }
  const auto padding = static_cast<float>(
      server::get().setting.config.get<"surface_grouping", "padding">());
  const auto spacing = static_cast<float>(
      server::get().setting.config.get<"surface_grouping", "window_spacing">());
  const auto decoration_height = static_cast<float>(server::get()
          .setting.config.get<"surface_grouping", "decoration_height">());

  width += (static_cast<float>(spacing) *
               static_cast<float>(this->children_.size() - 1)) +
           (2 * padding);
  this->geometry().scale() = glm::vec3{
      width,
      max_height + (2 * padding) + decoration_height,
      1.F,
  };

  // arrange children like a flexbox
  glm::vec3 next_surface_pos{
      -(width / 2.F) + padding,
      (decoration_height / 2.F),
      0.F,
  };
  for (auto& child : this->children_) {
    next_surface_pos.x += child->geometry().scale().x / 2.F;
    child->move_absolute(next_surface_pos);
    child->geometry().rotation() = glm::quat();
    next_surface_pos.x += (child->geometry().scale().x / 2.F) + spacing;
  }

  this->look_at_camera();
}

void ToplevelGroupNode::leave() {
  this->is_cursor_on_decoration_ = false;
}
void ToplevelGroupNode::motion(const glm::vec3& /*origin*/,
    const glm::vec3& /*direction*/, const glm::vec3& pos) {
  const auto distance_from_bottom = this->geometry().scale().y - pos.y;
  const auto decoration_height    = static_cast<float>(server::get()
          .setting.config.get<"surface_grouping", "decoration_height">());
  this->is_cursor_on_decoration_  = distance_from_bottom < decoration_height;
}
void ToplevelGroupNode::button(uint32_t button, bool is_pressed) {
  if (button != BTN_LEFT || !is_pressed) {
    return;
  }
  if (!this->is_cursor_on_decoration_) {
    return;
  }
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(
          this->shared_from_this()));
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
  const auto x = this->geometry().scale().x * result->u;
  const auto y = this->geometry().scale().y * (1.F - result->v);
  return SceneIntersection{
      .node           = this->shared_from_this(),
      .intersect_info = {.distance = result->distance,
                         .pos                     = glm::vec3{x, y, 0.F}},
  };
}

void ToplevelGroupNode::handle_child_erased() {
  if (this->children_.empty()) {
    server::get().enqueue_wayland_thread_task(
        [node = this->shared_from_this()] {
          core::server::get().workspace.current_scene().erase(node);
        });
  } else {
    this->reconfigure();
  }
}
}  // namespace yabe::core::scene
