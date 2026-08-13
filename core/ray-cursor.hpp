#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>
#include <numbers>

#include "common/common.hpp"
#include "common/unit.hpp"
#include "core/surface.hpp"

namespace yabe::core {
class RayCursor {
 public:
  DISABLE_MOVE_AND_COPY(RayCursor);
  explicit RayCursor() = default;
  ~RayCursor()         = default;

  void move_relative(float polar, float azimuth);
  void set_length(float length);
  void set_cursor(
      std::weak_ptr<Surface>&& surface, int32_t hotspot_x, int32_t hotspot_y);
  void set_cursor_rotation(glm::quat rotation);

  [[nodiscard]] const std::weak_ptr<Surface>& cursor_surface() {
    return this->cursor_surface_;
  }
  [[nodiscard]] glm::vec3 origin() const {
    return this->origin_;
  }
  [[nodiscard]] float length() const {
    return this->length_;
  }
  [[nodiscard]] glm::vec3 direction() const {
    return glm::rotate(this->rotation(), kBaseDirection);
  }
  [[nodiscard]] glm::mat4 cursor_model_mat() const;

 private:
  [[nodiscard]] glm::quat rotation() const;

  constexpr static glm::vec3 kBaseDirection = glm::vec3(0.F, 0.F, 1.F);

  glm::vec3 origin_  = {0.F, -0.3F, -0.001F};
  float     length_  = 5.F;
  float     polar_   = std::numbers::pi / 2.F;  // [0, pi]
  float     azimuth_ = std::numbers::pi;        // x-z plane, [0, 2pi]

  std::weak_ptr<Surface> cursor_surface_;
  glm::mat4              cursor_rotation_mat_ = glm::mat4(1.F);
  unit::Pixel            cursor_hotspot_x_{0};
  unit::Pixel            cursor_hotspot_y_{0};
};
}  // namespace yabe::core
