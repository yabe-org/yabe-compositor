#include "layer-manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <utility>

#include "common/box.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/server.hpp"
#include "protocols/wayland/output.hpp"
#include "protocols/wlr/layer-surface.hpp"  // IWYU pragma: keep

namespace yabe::core {
namespace {
/// @brief converts the rotation to the axis multiplied by the angle
/// @return the shortest rotation vector, in the range of [0, pi] radians
glm::vec3 to_rotation_vector(glm::quat q) {
  // q and -q represent the same rotation; choose the shorter one
  if (q.w < 0.F) {
    q = -q;
  }
  const glm::vec3 axis     = glm::vec3(q.x, q.y, q.z);
  const float     sin_half = glm::length(axis);
  if (sin_half < 1e-6F) {
    // angle / sin(angle / 2) approaches 2 as the angle approaches 0
    return 2.F * axis;
  }
  const float angle = 2.F * std::atan2(sin_half, q.w);
  return axis * (angle / sin_half);
}
glm::quat from_rotation_vector(const glm::vec3& v) {
  const float angle = glm::length(v);
  if (angle < 1e-6F) {
    return glm::normalize(glm::quat(1.F, v.x / 2.F, v.y / 2.F, v.z / 2.F));
  }
  return glm::angleAxis(angle, v / angle);
}

template <setting::schema::FixedString Side, setting::schema::FixedString Axis>
int resolve_margin(const auto& margin) {
  return margin.template get_if_set<Side>()
      .or_else([&margin] {
        return margin.template get_if_set<Axis>();
      })
      .value_or(margin.template get<"rest">());
}
common::Box2D viewport() {
  const auto& margin =
      server::get().setting.config.get<"fixed_layer", "margin">();
  const int  left          = resolve_margin<"left", "x">(margin);
  const int  right         = resolve_margin<"right", "x">(margin);
  const int  top           = resolve_margin<"top", "y">(margin);
  const int  bottom        = resolve_margin<"bottom", "y">(margin);
  const auto viewport_size = proto::wayland::output::get_viewport_size();
  // setting zero to width/height means that the client can decide its size,
  // so fallback to 1 in order to prevent it
  return {
      .x{left},
      .y{top},
      .width{std::max(1, viewport_size.width - (left + right))},
      .height{std::max(1, viewport_size.height - (top + bottom))},
  };
}
}  // namespace

void LayerManager::add(StoredLayerSurface&& surface) {
  auto layer = static_cast<size_t>(surface->layer());
  surfaces_.at(layer).emplace(surface->resource(), std::move(surface));
}
void LayerManager::remove(wl_resource* resource) {
  for (auto& layer : surfaces_) {
    auto it = layer.find(resource);
    if (it != layer.end()) {
      layer.erase(it);
      return;
    }
  }
}

std::optional<LayerManager::StoredLayerSurface> LayerManager::get(
    wl_resource* resource) const {
  for (const auto& layer : surfaces_) {
    auto it = layer.find(resource);
    if (it != layer.end()) {
      return it->second;
    }
  }
  return std::nullopt;
}

void LayerManager::reconfigure_all() const {
  const auto entire_area = viewport();
  for (const auto& layer : this->surfaces_) {
    auto available_area = entire_area;
    for (const auto& surface : layer) {
      surface.second->reconfigure(entire_area, available_area);
    }
  }
}
void LayerManager::reconfigure_layer(Layer layer) const {
  const auto entire_area    = viewport();
  auto       available_area = entire_area;
  for (const auto& surface : this->surfaces_.at(static_cast<size_t>(layer))) {
    surface.second->reconfigure(entire_area, available_area);
  }
}

void LayerManager::update_rotation(const glm::mat4& view_mat) {
  const auto now         = std::chrono::steady_clock::now();
  // view_mat consists of only rotation and translation,
  // so its upper-left 3x3 is the transposed eye rotation
  this->target_rotation_ = glm::quat_cast(glm::transpose(glm::mat3(view_mat)));

  if (this->should_skip_smoothing_) {
    this->rotation_              = this->target_rotation_;
    this->angular_velocity_      = glm::vec3(0.F);
    this->is_following_          = false;
    this->should_skip_smoothing_ = false;
    this->last_rotation_update_  = now;
    return;
  }

  const float dt_sec =
      std::chrono::duration<float>(now - this->last_rotation_update_).count();
  this->last_rotation_update_ = now;

  // start following when the diff(head rotate between the layers) > start angle
  constexpr float kStartFollowingAngle = glm::radians(1.5F);
  // stop following iff both angle and speed are short enough
  // use both in order to avoid stopping following
  // while speed is still high or angle is too differ
  constexpr float kStopFollowingAngle  = glm::radians(0.7F);
  constexpr float kStopFollowingSpeed  = glm::radians(0.5F);  // [rad/s]
  // offset of the layers from the target rotation
  // as a rotation vector in the world space
  glm::vec3       offset               = to_rotation_vector(
      this->rotation_ * glm::inverse(this->target_rotation_));
  const float angle = glm::length(offset);
  if (!this->is_following_ && angle > kStartFollowingAngle) {
    this->is_following_     = true;
    // start from zero velocity to ease in
    this->angular_velocity_ = glm::vec3(0.F);
  } else if (this->is_following_ && angle < kStopFollowingAngle &&
             glm::length(this->angular_velocity_) < kStopFollowingSpeed) {
    this->is_following_     = false;
    this->angular_velocity_ = glm::vec3(0.F);
  }
  if (!this->is_following_) {
    return;
  }
  // exact solution of the critically damped spring
  //   x'' = -w^2 x - 2w x'
  // (x = offset and x' = angular_velocity_)
  // is: x(t) = e^(-wt) * {x_0 + (v_0 + wx_0)t}
  //     v(t) = e^(-wt) * {v_0 - (v_0 + wx_0)wt}
  // and v_0 = 0 when is_following becomes true
  const double w =
      server::get().setting.config.get<"fixed_layer", "follow_speed">();
  const double    decay   = std::exp(-w * dt_sec);
  // k = v_0 - (v_0 + wx_0)wt
  const glm::vec3 k       = (this->angular_velocity_ + (w * offset)) * dt_sec;
  offset                  = (offset + k) * decay;
  this->angular_velocity_ = (this->angular_velocity_ - (w * k)) * decay;
  this->rotation_ = from_rotation_vector(offset) * this->target_rotation_;
}

void LayerManager::reset_rotation() {
  this->should_skip_smoothing_ = true;
}

void LayerManager::draw_under_scene(
    const glm::mat4& proj_mat, const glm::mat4& view_mat) const {
  const auto eye_from_layer_mat = this->eye_from_layer_mat(view_mat);
  this->draw_layer(Layer::BACKGROUND, proj_mat, eye_from_layer_mat);
  this->draw_layer(Layer::BOTTOM, proj_mat, eye_from_layer_mat);
}
void LayerManager::draw_above_scene(
    const glm::mat4& proj_mat, const glm::mat4& view_mat) const {
  const auto eye_from_layer_mat = this->eye_from_layer_mat(view_mat);
  this->draw_layer(Layer::UNSPECIFIED, proj_mat, eye_from_layer_mat);
  this->draw_layer(Layer::TOP, proj_mat, eye_from_layer_mat);
  this->draw_layer(Layer::OVERLAY, proj_mat, eye_from_layer_mat);
}

glm::mat4 LayerManager::eye_from_layer_mat(const glm::mat4& view_mat) const {
  // rotate the layers around the eye without translation, so that both eyes
  // still render the layers at the same NDC position (no disparity)
  return {glm::mat3(view_mat) * glm::mat3_cast(this->rotation_)};
}

void LayerManager::draw_layer(Layer layer, const glm::mat4& proj_mat,
    const glm::mat4& eye_from_layer_mat) const {
  const auto viewport_size   = proto::wayland::output::get_viewport_size();
  const auto viewport_width  = static_cast<float>(viewport_size.width);
  const auto viewport_height = static_cast<float>(viewport_size.height);

  for (const auto& layer_surface : surfaces_.at(static_cast<size_t>(layer))) {
    if (!layer_surface.second->is_mapped()) {
      continue;
    }
    const auto& surface = layer_surface.second->wl_surface();
    if (!surface) {
      continue;
    }
    // box is top-left origin and [0, viewport_w&h] range, so
    // convert to center origin and [-1, 1] range
    // for OpenGL (Normalized Device Coordinates; NDC)
    const auto box = layer_surface.second->box();
    const auto center_x =
        static_cast<float>(*box.x) + (static_cast<float>(*box.width) / 2.F);
    auto x = center_x / viewport_width;  // [0, 1]
    x *= 2.F;                            // [0, 2]
    x -= 1.F;                            // [-1, 1]

    const auto center_y =
        static_cast<float>(*box.y) + (static_cast<float>(*box.height) / 2.F);
    auto y = center_y / viewport_height;  // [0, 1]
    y *= 2.F;                             // [0, 2]
    y = 1.F - y;                          // [-1, 1], invert Y

    const auto projection_scale_x = 1.F / glm::column(proj_mat, 0).x;
    const auto projection_scale_y = 1.F / glm::column(proj_mat, 1).y;
    glm::mat4  model_mat{1.F};
    // place the layer on the z = -1 eye-space plane so that the
    // projection matrix does not scale the calculated NDC size
    model_mat = glm::translate(model_mat,
        glm::vec3{x * projection_scale_x, y * projection_scale_y, -1.F});
    // NDC is [-1, 1] range, so double the size
    model_mat =
        glm::scale(model_mat, glm::vec3{
                                  2.F * static_cast<float>(*box.width) /
                                      viewport_width * projection_scale_x,
                                  2.F * static_cast<float>(*box.height) /
                                      viewport_height * projection_scale_y,
                                  1.F,
                              });
    surface->upload_texture_if_needed();
    server::get().rect_renderer.render(
        proj_mat * eye_from_layer_mat * model_mat, surface->texture());
  }
}
}  // namespace yabe::core
