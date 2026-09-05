#include "layer-manager.hpp"

#include <cstdlib>
#include <glm/glm.hpp>

#include "common/box.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/server.hpp"
#include "protocols/wayland/output.hpp"
#include "protocols/wlr/layer-surface.hpp"

namespace yabe::core {
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

void LayerManager::reconfigure(Layer layer) const {
  common::Box2D available_area{
      .x{0},
      .y{0},
      .width{proto::wayland::output::get_viewport_width()},
      .height{proto::wayland::output::get_viewport_height()},
  };
  for (const auto& surface : surfaces_.at(static_cast<size_t>(layer))) {
    surface.second->reconfigure(available_area);
  }
}

void LayerManager::draw_under_scene(const glm::mat4& proj_mat) const {
  this->draw_layer(Layer::BACKGROUND, proj_mat);
  this->draw_layer(Layer::BOTTOM, proj_mat);
}
void LayerManager::draw_above_scene(const glm::mat4& proj_mat) const {
  this->draw_layer(Layer::UNSPECIFIED, proj_mat);
  this->draw_layer(Layer::TOP, proj_mat);
  this->draw_layer(Layer::OVERLAY, proj_mat);
}

void LayerManager::draw_layer(Layer layer, const glm::mat4& proj_mat) const {
  const auto viewport_width =
      static_cast<float>(proto::wayland::output::get_viewport_width());
  const auto viewport_height =
      static_cast<float>(proto::wayland::output::get_viewport_height());

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

    const auto projection_scale_x = 1.F / proj_mat[0][0];
    const auto projection_scale_y = 1.F / proj_mat[1][1];
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
        proj_mat * model_mat, surface->texture());
  }
}
}  // namespace yabe::core
