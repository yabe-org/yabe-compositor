#include "toplevel-selection.hpp"

#include <linux/input-event-codes.h>

#include <mutex>

#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
inline ToplevelSelectionPointerGrab ToplevelSelectionPointerGrab::instance;

ToplevelSelectionPointerGrab* ToplevelSelectionPointerGrab::start() {
  {
    const std::lock_guard<std::mutex> lock(instance.lasso_points_mutex_);
    instance.lasso_points_.clear();
    instance.lasso_points_.emplace_back(server::get().ray_cursor.ray_state());
  }
  instance.last_finished_at_ = std::nullopt;
  return &instance;
}

void ToplevelSelectionPointerGrab::cleanup() {
  this->last_finished_at_ = std::chrono::steady_clock::now();
  server::get().seat.pointer.focus_pointed_surface();
}

void ToplevelSelectionPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(dy, dx);
  const auto& prev_point    = this->lasso_points_.back();
  const auto& new_point     = server::get().ray_cursor.ray_state();
  const auto  delta_polar   = new_point.polar - prev_point.polar;
  const auto  delta_azimuth = new_point.azimuth - prev_point.azimuth;
  if ((delta_polar * delta_polar) + (delta_azimuth * delta_azimuth) >
      0.00006F) {
    const std::lock_guard<std::mutex> lock(this->lasso_points_mutex_);
    this->lasso_points_.emplace_back(new_point);
  }
}

void ToplevelSelectionPointerGrab::button(uint32_t button, bool is_pressed) {
  if (button != BTN_RIGHT || is_pressed) {
    return;
  }
  server::get().seat.pointer.end_grab();
}

void ToplevelSelectionPointerGrab::draw(
    render::LassoRenderer& lasso_renderer, const glm::mat4& vp_mat) {
  float alpha = 1.0F;
  if (instance.last_finished_at_) {
    const auto elapsed =
        std::chrono::steady_clock::now() - *instance.last_finished_at_;
    std::chrono::milliseconds fadeout_duration_ms{server::get()
            .setting.config.get<"surface_grouping", "fadeout_duration_ms">()};
    if (elapsed > fadeout_duration_ms) {
      return;
    }
    const auto progress =
        elapsed / std::chrono::duration<float>{fadeout_duration_ms};
    alpha = 1.0F - static_cast<float>(progress);
  }

  const std::lock_guard<std::mutex> lock(instance.lasso_points_mutex_);
  // Lasso draws lines so at least 2 points are required
  if (instance.lasso_points_.size() < 2) {
    return;
  }
  lasso_renderer.render(vp_mat, instance.lasso_points_, alpha);
}
}  // namespace yabe::core::server_seat::pointer_grab
