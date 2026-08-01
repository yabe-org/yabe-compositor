#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <numbers>

#include "common/common.hpp"
#include "common/unit.hpp"

namespace yabe::core {
class RayCursor {
 public:
  DISABLE_MOVE_AND_COPY(RayCursor);
  explicit RayCursor() = default;
  ~RayCursor()         = default;

  void move_relative(float polar, float azimuth);
  void set_length(float length);
  void set_cursor_hotspot(int32_t x, int32_t y);

  [[nodiscard]] glm::mat4 cursor_model_mat(
      unit::Pixel width, unit::Pixel height) const;

  [[nodiscard]] glm::vec3 origin() const {
    return this->origin_;
  }
  [[nodiscard]] glm::quat rotation() const {
    return glm::angleAxis(this->azimuth_, glm::vec3{0.F, 1.F, 0.F}) *
           glm::angleAxis((std::numbers::pi_v<float> / 2.F) - this->polar_,
               glm::vec3{-1.F, 0.F, 0.F});
  }
  [[nodiscard]] float length() const {
    return this->length_;
  }
  [[nodiscard]] glm::vec3 direction() const {
    return glm::rotate(this->rotation(), kBaseDirection);
  }

 private:
  constexpr static glm::vec3 kBaseDirection = glm::vec3(0.F, 0.F, 1.F);

  glm::vec3 origin_  = {0.F, -0.3F, -0.001F};
  float     length_  = 5.F;
  float     polar_   = std::numbers::pi / 2.F;  // [0, pi]
  float     azimuth_ = std::numbers::pi;        // x-z plane, [0, 2pi]

  unit::Pixel cursor_hotspot_x_{0};
  unit::Pixel cursor_hotspot_y_{0};
};
}  // namespace yabe::core
