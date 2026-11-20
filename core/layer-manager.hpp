#pragma once

#include <wayland-server-core.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <map>
#include <memory>

#include "common/common.hpp"

namespace yabe::proto::wlr::layer_shell::layer_surface {
class LayerSurface;
}

namespace yabe::core {
enum class Layer : uint8_t {
  BACKGROUND,
  BOTTOM,
  UNSPECIFIED,
  TOP,
  OVERLAY,
  LAYER_COUNT,
};

class LayerManager {
 public:
  using StoredLayerSurface =
      std::shared_ptr<proto::wlr::layer_shell::layer_surface::LayerSurface>;

  DISABLE_MOVE_AND_COPY(LayerManager);
  explicit LayerManager() = default;
  ~LayerManager()         = default;

  void                              add(StoredLayerSurface&& surface);
  void                              remove(wl_resource* resource);
  std::optional<StoredLayerSurface> get(wl_resource* resource) const;
  void                              reconfigure(Layer layer) const;

  /// updates the stabilized layer rotation that lazily follows the head
  /// rotation; may be called for each eye because the update amount depends
  /// on the elapsed time
  void update_rotation(const glm::mat4& view_mat);
  /// makes the next update_rotation() snap to the head rotation
  void reset_rotation();

  void draw_under_scene(
      const glm::mat4& proj_mat, const glm::mat4& view_mat) const;
  void draw_above_scene(
      const glm::mat4& proj_mat, const glm::mat4& view_mat) const;

 private:
  void                    draw_layer(Layer layer, const glm::mat4& proj_mat,
      const glm::mat4& eye_from_layer_mat) const;
  [[nodiscard]] glm::mat4 eye_from_layer_mat(const glm::mat4& view_mat) const;

  glm::quat rotation_              = glm::quat(1.F, 0.F, 0.F, 0.F);
  glm::quat target_rotation_       = glm::quat(1.F, 0.F, 0.F, 0.F);
  /// time derivative of the rotation vector of the layers in the world space
  glm::vec3 angular_velocity_      = glm::vec3(0.F);
  bool      should_skip_smoothing_ = true;
  bool      is_following_          = false;
  std::chrono::steady_clock::time_point last_rotation_update_;

  std::array<  //
      std::map<wl_resource*, StoredLayerSurface>,
      static_cast<size_t>(Layer::LAYER_COUNT)>
      surfaces_;
};
}  // namespace yabe::core
