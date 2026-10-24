#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>

#include "common/common.hpp"
#include "common/unit.hpp"
#include "core/render/texture.hpp"
#include "core/scene/scene-node.hpp"
#include "core/scene/surface-node.hpp"
#include "core/surface.hpp"

namespace yabe::core {
struct RayState {
  float polar;
  float azimuth;
};

class RayCursor {
 public:
  DISABLE_MOVE_AND_COPY(RayCursor);
  explicit RayCursor() = default;
  ~RayCursor()         = default;

  void move_relative(float polar, float azimuth);
  void set_length(float length);
  void set_drag_and_drop_icon(std::weak_ptr<Surface>&& surface);
  void set_cursor(
      std::weak_ptr<Surface>&& surface, int32_t hotspot_x, int32_t hotspot_y);
  void set_cursor_rotation(glm::quat rotation);
  void reset_cursor();

  void draw_cursor(const glm::mat4& vp_mat);
  void invalidate_graphics();

  [[nodiscard]] glm::vec3 origin() const {
    return this->origin_;
  }
  [[nodiscard]] float length() const {
    return this->length_;
  }
  [[nodiscard]] glm::vec3 direction() const {
    return glm::rotate(this->rotation(), kBaseDirection);
  }
  [[nodiscard]] const RayState& ray_state() const {
    return this->state_;
  }

  constexpr static glm::vec3 kBaseDirection = glm::vec3(0.F, 0.F, -1.F);

 private:
  [[nodiscard]] glm::quat rotation() const;
  [[nodiscard]] glm::mat4 cursor_model_mat() const;
  [[nodiscard]] glm::mat4 drag_and_drop_icon_model_mat() const;

  constexpr static float kDefaultLength = scene::kSceneNodeDistance + 0.1F;

  glm::vec3 origin_ = {0.F, -0.3F, -0.001F};
  float     length_ = kDefaultLength;
  RayState  state_  = {
      .polar   = 0,
      .azimuth = 0,  // x-z plane, [0, 2pi]
  };

  bool            is_default_cursor_texture_uploaded_ = false;
  render::Texture default_cursor_texture_;
  // nullopt              : uses default cursor texture
  // has value but nullptr: invisible cursor
  std::optional<std::weak_ptr<Surface>> cursor_surface_;
  glm::mat4                             cursor_rotation_mat_ = glm::mat4(1.F);
  common::unit::Pixel                   cursor_hotspot_x_{0};
  common::unit::Pixel                   cursor_hotspot_y_{0};

  std::weak_ptr<Surface> drag_and_drop_icon_surface_;
};
}  // namespace yabe::core
