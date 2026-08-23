#pragma once

#include <wayland-server-core.h>

#include <array>
#include <cstddef>
#include <map>
#include <memory>

#include "common/common.hpp"
#include "core/render/rect-renderer.hpp"

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
  void                              reconfigure(wl_resource* resource) const;

  void draw_under_scene(render::DrawRectFunc draw_rect) const;
  void draw_above_scene(render::DrawRectFunc draw_rect) const;

 private:
  void draw_layer(Layer layer, render::DrawRectFunc draw_rect) const;
  std::array<  //
      std::map<wl_resource*, StoredLayerSurface>,
      static_cast<size_t>(Layer::LAYER_COUNT)>
      surfaces_;
};
}  // namespace yabe::core
