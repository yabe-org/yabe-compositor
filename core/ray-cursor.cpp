#include "ray-cursor.hpp"

#include <wayland-server-protocol.h>

#include <glm/ext.hpp>
#include <memory>
#include <numbers>

#include "common/coordinate.hpp"
#include "common/unit.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::core {
namespace {
constexpr int        kDefaultCursorSize    = 32;
constexpr std::array kDefaultCursorTexture = []() {
  std::array<uint8_t,
      static_cast<size_t>(kDefaultCursorSize * kDefaultCursorSize * 4)>
      data{};
  for (int y = 0; y < kDefaultCursorSize; ++y) {
    for (int x = 0; x < kDefaultCursorSize; ++x) {
      // 0 to 1
      const auto dx =
          static_cast<double>(x) / static_cast<double>(kDefaultCursorSize);
      const auto y0 = (-7.0 * dx) + 0.0;
      const auto y1 = (-1.1 * dx) + 0.0;
      const auto y2 =
          (0.55 * (dx - (1.0 / 7.0))) - 1.0;  // intersects y0 at y = -1
      // 0 to -1
      const auto dy =
          -static_cast<double>(y) / static_cast<double>(kDefaultCursorSize);
      const int index = ((y * kDefaultCursorSize) + x) * 4;
      if (dy > y0 && dy < y1 && dy > y2) {
        data.at(index + 0) = 0xff;  // B
        data.at(index + 1) = 0xff;  // G
        data.at(index + 2) = 0xff;  // R
        data.at(index + 3) = 0xff;  // A
      } else {
        data.at(index + 0) = 0x00;  // B
        data.at(index + 1) = 0x00;  // G
        data.at(index + 2) = 0x00;  // R
        data.at(index + 3) = 0x00;  // A
      }
    }
  }
  return data;
}();
}  // namespace

RayState::RayState(const glm::vec3& direction) {
  const auto spherical =
      common::coordinate::CartesianCoord(direction).to_spherical();
  this->polar   = spherical.polar;
  this->azimuth = spherical.azimuth;
}

void RayCursor::move_relative(float polar, float azimuth) {
  this->state_.azimuth += azimuth;
  if (this->state_.azimuth > std::numbers::pi) {
    this->state_.azimuth -= std::numbers::pi * 2.F;
  } else if (this->state_.azimuth < -std::numbers::pi) {
    this->state_.azimuth += std::numbers::pi * 2.F;
  }
  constexpr float kPolarMax = std::numbers::pi / 2.F * 0.95F;
  this->state_.polar =
      std::clamp(this->state_.polar + polar, -kPolarMax, kPolarMax);
}

void RayCursor::set_length(float length) {
  this->length_ = length;
}
void RayCursor::set_drag_and_drop_icon(std::weak_ptr<Surface>&& surface) {
  this->drag_and_drop_icon_surface_ = std::move(surface);
}
void RayCursor::set_cursor(
    std::weak_ptr<Surface>&& surface, int32_t x, int32_t y) {
  this->cursor_surface_   = std::move(surface);
  this->cursor_hotspot_x_ = common::unit::Pixel(x);
  this->cursor_hotspot_y_ = common::unit::Pixel(y);
}
void RayCursor::reset_cursor() {
  this->cursor_surface_   = std::nullopt;
  this->cursor_hotspot_x_ = 0;
  this->cursor_hotspot_y_ = 0;
  this->set_length(kDefaultLength);
}

void RayCursor::draw_cursor(const glm::mat4& vp_mat) {
  const auto drag_and_drop_icon_surface =
      this->drag_and_drop_icon_surface_.lock();
  if (drag_and_drop_icon_surface) {
    drag_and_drop_icon_surface->upload_texture_if_needed();
    server::get().rect_renderer.render(
        vp_mat * this->drag_and_drop_icon_model_mat(),
        drag_and_drop_icon_surface->texture());
  }

  if (this->cursor_surface_) {
    if (auto cursor_surface = this->cursor_surface_.value().lock()) {
      cursor_surface->upload_texture_if_needed();
      server::get().rect_renderer.render(
          vp_mat * this->cursor_model_mat(), cursor_surface->texture());
    }
  } else {
    if (!this->is_default_cursor_texture_uploaded_) {
      common::unit::Pixel size{kDefaultCursorSize};
      this->default_cursor_texture_.upload(
          // NOLINTNEXTLINE: (static|const|reinterpret)_cast cannot cast this (why?)
          (void*)(kDefaultCursorTexture.data()), WL_SHM_FORMAT_ARGB8888, size,
          size, kDefaultCursorSize * 4);
      this->is_default_cursor_texture_uploaded_ = true;
    }
    server::get().rect_renderer.render(
        vp_mat * this->cursor_model_mat(), this->default_cursor_texture_);
  }
}

void RayCursor::invalidate_graphics() {
  this->default_cursor_texture_.invalidate();
  this->is_default_cursor_texture_uploaded_ = false;
}

glm::quat RayCursor::rotation() const {
  return glm::angleAxis(-this->state_.azimuth, glm::vec3{0.F, 1.F, 0.F}) *
         glm::angleAxis(+this->state_.polar, glm::vec3{1.F, 0.F, 0.F});
}
glm::mat4 RayCursor::rotation_mat() const {
  const auto node = server::get().seat.pointer.get_focused_scene_node().lock();
  if (!node) {
    return glm::toMat4(this->rotation());
  }
  const auto lock = scene::Scene::mutex.lock();
  // delete position component
  return glm::column(node->world_transform(), 3, glm::vec4{0.F, 0.F, 0.F, 1.F});
}
glm::mat4 RayCursor::cursor_model_mat() const {
  common::unit::Pixel width{0};
  common::unit::Pixel height{0};
  if (this->cursor_surface_) {
    if (auto surface = this->cursor_surface_.value().lock()) {
      width  = surface->texture_width();
      height = surface->texture_height();
    }
  } else {
    width  = kDefaultCursorSize;
    height = kDefaultCursorSize;
  }
  const auto pos = this->origin_ + this->direction() * this->length_;
  // In 2D, surface position is calculated by $pos - hotspot$
  // but yabe uses OpenGL, so Y axis should be inverted
  // and the surface position should be the center of the surface
  const auto offset_x =
      common::unit::Pixel{width.value / 2} - this->cursor_hotspot_x_;
  const auto offset_y =
      (common::unit::Pixel{height.value / 2} - this->cursor_hotspot_y_) * -1;
  const auto hotspot_offset =
      glm::vec3(*offset_x.to_world_unit(), *offset_y.to_world_unit(), 0.F);

  // hotspot_offset is in surface-local coordinates, so apply it after rotation
  return glm::translate(glm::mat4(1.F), pos) * this->rotation_mat() *
         glm::translate(glm::mat4(1.F), hotspot_offset) *
         glm::scale(glm::mat4(1.F),
             glm::vec3(*width.to_world_unit(), *height.to_world_unit(), 0.F));
}
glm::mat4 RayCursor::drag_and_drop_icon_model_mat() const {
  const auto surface = this->drag_and_drop_icon_surface_.lock();
  const auto pos     = this->origin_ + this->direction() * this->length_;
  const auto offset  = glm::vec3{
      +(*surface->texture_width().to_world_unit()) / 2.F,
      -(*surface->texture_height().to_world_unit()) / 2.F,
      0.F,
  };

  return glm::translate(glm::mat4(1.F), pos) * this->rotation_mat() *
         glm::translate(glm::mat4(1.F), offset) *
         glm::scale(
             glm::mat4(1.F), glm::vec3{
                                 *surface->texture_width().to_world_unit(),
                                 *surface->texture_height().to_world_unit(),
                                 0.F,
                             });
}
}  // namespace yabe::core
