#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wlr-layer-shell-protocol.h>

#include <cstdint>

#include "common/box.hpp"
#include "common/common.hpp"
#include "common/edge.hpp"
#include "common/margin.hpp"
#include "common/unit.hpp"
#include "core/layer-manager.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wlr::layer_shell::layer_surface {
class LayerSurface : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(LayerSurface);
  LayerSurface(wl_resource*           resource,
      std::shared_ptr<core::Surface>& wl_surface, std::string&& name_space);
  ~LayerSurface() override;

  void                                          commit() override;
  [[nodiscard]] std::shared_ptr<core::Toplevel> get_toplevel() override;

  void reconfigure(
      const common::Box2D& entire_area, common::Box2D& available_area);

  void set_size(uint32_t width, uint32_t height);
  void set_anchor(uint32_t anchor);
  void set_exclusive_zone(int32_t zone);
  void set_margin(int32_t top, int32_t right, int32_t bottom, int32_t left);
  void set_keyboard_interactivity(
      zwlr_layer_surface_v1_keyboard_interactivity interactivity);
  void set_layer(zwlr_layer_shell_v1_layer layer);
  void set_exclusive_edge(zwlr_layer_surface_v1_anchor edge);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::shared_ptr<core::Surface> wl_surface() const {
    return this->wl_surface_.lock();
  }
  [[nodiscard]] core::Layer layer() const {
    return this->current_.layer;
  }
  [[nodiscard]] const common::Box2D& box() const {
    return this->box_;
  }
  [[nodiscard]] bool is_mapped() const {
    return this->state_ == State::MAPPED;
  }

 private:
  struct {
    common::unit::Pixel                          requested_width{0};
    common::unit::Pixel                          requested_height{0};
    common::Edge                                 anchor;
    int32_t                                      exclusive_zone = 0;
    common::Margin                               margin;
    zwlr_layer_surface_v1_keyboard_interactivity keyboard_interactivity;
    core::Layer layer = core::Layer::UNSPECIFIED;
  } pending_, current_;
  std::optional<zwlr_layer_surface_v1_anchor> exclusive_edge_;

  enum class State : uint8_t {
    INITIAL,
    SENT_FIRST_CONFIGURE,
    MAPPED,
  } state_ = State::INITIAL;
  common::Box2D box_;

  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> wl_surface_;
  std::string                  name_space_;
};

void create(wl_client* client, int version, uint32_t id,
    std::shared_ptr<core::Surface>& wl_surface, const char* name_space);
}  // namespace yabe::proto::wlr::layer_shell::layer_surface
