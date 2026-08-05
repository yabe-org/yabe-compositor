#pragma once

#include <wayland-server.h>

#include <cstdint>
#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"
#include "protocols/xdg-shell/xdg-positioner.hpp"
#include "protocols/xdg-shell/xdg-surface.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_popup {
class XdgPopup : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(XdgPopup);
  XdgPopup(wl_resource*                         resource,
      std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
      std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
      xdg_positioner::XdgPositioner*            positioner);
  ~XdgPopup() override = default;

  void commit() override;
  void set_activated(bool is_activated) override;

  /// @param token pass value if it is called via protocol,
  /// otherwise (e.g. initialization) pass std::nullopt
  void reposition(std::optional<uint32_t> token);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] std::weak_ptr<xdg_surface::XdgSurface> popup_surface() const {
    return this->popup_surface_;
  }
  [[nodiscard]] std::weak_ptr<xdg_surface::XdgSurface> parent_surface() const {
    return this->parent_surface_;
  }

 private:
  wl_resource*                                   resource_;
  std::weak_ptr<xdg_surface::XdgSurface>         popup_surface_;
  std::weak_ptr<xdg_surface::XdgSurface>         parent_surface_;
  std::unique_ptr<xdg_positioner::XdgPositioner> positioner_;

  bool is_initial_commit_sent_ = false;
};

void create(wl_client* client, uint32_t id,
    std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
    std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
    xdg_positioner::XdgPositioner*            positioner);
}  // namespace yabe::proto::xdg_shell::xdg_popup
