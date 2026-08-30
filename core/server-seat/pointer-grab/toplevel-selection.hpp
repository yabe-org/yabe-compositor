#pragma once

#include <chrono>
#include <glm/glm.hpp>
#include <vector>

#include "common/common.hpp"
#include "core/ray-cursor.hpp"
#include "core/render/lasso-renderer.hpp"
#include "pointer-grab.hpp"

namespace yabe::core::server_seat::pointer_grab {
class ToplevelSelectionPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(ToplevelSelectionPointerGrab);

  static ToplevelSelectionPointerGrab* start();

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

  static void draw(
      render::LassoRenderer& lasso_renderer, const glm::mat4& vp_mat);

 private:
  // should be declared here because draw() needs to access lasso_points_
  static ToplevelSelectionPointerGrab instance;

  explicit ToplevelSelectionPointerGrab() = default;
  ~ToplevelSelectionPointerGrab()         = default;

  std::optional<std::chrono::steady_clock::time_point> last_finished_at_;

  std::mutex            lasso_points_mutex_;
  std::vector<RayState> lasso_points_;
};
}  // namespace yabe::core::server_seat::pointer_grab
