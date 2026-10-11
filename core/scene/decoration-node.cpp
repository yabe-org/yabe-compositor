#include "decoration-node.hpp"

#include <linux/input-event-codes.h>
#include <wayland-server-protocol.h>

#include <glm/ext.hpp>

#include "common/unit.hpp"
#include "core/server-seat/pointer-grab/move.hpp"
#include "core/server.hpp"
#include "core/setting/fields/config.hpp"

namespace yabe::core::scene {
namespace {
constexpr std::array<uint8_t, 4> kTextureColor{
    0x79, 0xc3, 0x98, 0xFF};  // ARGB, little endian
}  // namespace

void DecorationNode::button(uint32_t button, bool is_pressed) {
  if (button != BTN_LEFT || !is_pressed) {
    return;
  }
  const auto parent = this->parent().lock();
  if (!parent) {
    return;
  }
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(parent));
}

std::shared_ptr<SceneNode> DecorationNode::activation_target() {
  const auto parent = this->parent().lock();
  if (!parent) {
    return nullptr;
  }
  return parent->activation_target();
}

void DecorationNode::prepare_for_draw() {
  if (!this->is_texture_uploaded_) {
    const common::unit::Pixel size{1};
    this->texture_.upload(  // NOLINTNEXTLINE(google-readability-casting)
        (void*)(&kTextureColor), WL_SHM_FORMAT_ARGB8888, size, size, 1);
    this->is_texture_uploaded_ = true;
  }
  // update geometry here because it's difficult to detect
  // when the node size is changed in current implementation
  // ex: XdgToplevel has a DecorationNode but the Surface size is
  //     not yet changed at XdgToplevel::commit()
  const auto parent = this->parent().lock();
  if (!parent) {
    return;
  }
  const common::unit::Pixel offset(
      server::get().setting.config.get<"decoration", "offset">());
  const common::unit::Pixel height(
      server::get().setting.config.get<"decoration", "height">());
  this->geometry().pos() = glm::vec3{
      0.F,
      -(parent->geometry().scale().y / 2.F) - (*height.to_world_unit() / 2.F) -
          *offset.to_world_unit(),
      0.F,
  };
  this->geometry().scale() = glm::vec3{
      parent->geometry().scale().x,
      *height.to_world_unit(),
      1.F,
  };
}
void DecorationNode::draw_self(const glm::mat4& proj_mat,
    const glm::mat4& view_mat, const glm::mat4& world_transform) {
  const auto parent = this->parent().lock();
  if (!parent) {
    return;
  }
  // if the parent is owned by another parent
  // (ex: Toplevel is wrapped by ToplevelGroup),
  // decoration should be hidden
  if (parent->parent().lock()) {
    return;
  }
  const auto model_mat = proj_mat * view_mat * world_transform *
                         glm::scale(glm::mat4(1.F), this->geometry().scale());
  server::get().rect_renderer.render(model_mat, this->texture_);
}

std::optional<SceneIntersection> DecorationNode::check_intersection_self(
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
}  // namespace yabe::core::scene
