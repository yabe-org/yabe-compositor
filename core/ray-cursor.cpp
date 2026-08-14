#include "ray-cursor.hpp"

#include <glm/ext.hpp>
#include <memory>

#include "core/surface.hpp"

namespace yabe::core {
void RayCursor::move_relative(float polar, float azimuth) {
  this->azimuth_ += azimuth;

  constexpr float kPolarMin = std::numbers::pi / 8.F;
  constexpr float kPolarMax = std::numbers::pi - kPolarMin;
  this->polar_ = std::clamp(this->polar_ + polar, kPolarMin, kPolarMax);
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
  this->cursor_hotspot_x_ = unit::Pixel(x);
  this->cursor_hotspot_y_ = unit::Pixel(y);
}
void RayCursor::set_cursor_rotation(glm::quat rotation) {
  this->cursor_rotation_mat_ = glm::mat4_cast(rotation);
}

glm::quat RayCursor::rotation() const {
  return glm::angleAxis(this->azimuth_, glm::vec3{0.F, 1.F, 0.F}) *
         glm::angleAxis((std::numbers::pi_v<float> / 2.F) - this->polar_,
             glm::vec3{-1.F, 0.F, 0.F});
}
glm::mat4 RayCursor::cursor_model_mat() const {
  const auto width    = this->cursor_surface_.lock()->texture_width();
  const auto height   = this->cursor_surface_.lock()->texture_height();
  const auto pos      = this->origin_ + this->direction() * this->length_;
  // In 2D, surface position is calculated by $pos - hotspot$
  // but yabe uses OpenGL, so Y axis should be inverted
  // and the surface position should be the center of the surface
  const auto offset_x = unit::Pixel{width.value / 2} - this->cursor_hotspot_x_;
  const auto offset_y =
      (unit::Pixel{height.value / 2} - this->cursor_hotspot_y_) * -1;
  const auto hotspot_offset =
      glm::vec3(*offset_x.to_world_unit(), *offset_y.to_world_unit(), 0.F);

  return glm::translate(glm::mat4(1.F), pos + hotspot_offset) *
         this->cursor_rotation_mat_ *
         glm::scale(glm::mat4(1.F),
             glm::vec3(*width.to_world_unit(), *height.to_world_unit(), 0.F));
}
glm::mat4 RayCursor::drag_and_drop_icon_model_mat() const {
  const auto surface = this->drag_and_drop_icon_surface_.lock();
  const auto pos     = this->origin_ + this->direction() * this->length_;
  const auto offset  = glm::vec3{
      +(*surface->texture_width().to_world_unit()),
      -(*surface->texture_height().to_world_unit()),
      0.F,
  };

  return glm::translate(glm::mat4(1.F), pos + offset) *
         this->cursor_rotation_mat_ *
         glm::scale(
             glm::mat4(1.F), glm::vec3{
                                 *surface->texture_width().to_world_unit(),
                                 *surface->texture_height().to_world_unit(),
                                 0.F,
                             });
}
}  // namespace yabe::core
