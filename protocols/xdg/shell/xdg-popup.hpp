#pragma once

#include <wayland-server.h>

#include <cstdint>
#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"
#include "core/toplevel.hpp"
#include "protocols/xdg/shell/xdg-positioner.hpp"
#include "protocols/xdg/shell/xdg-surface.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg::shell::xdg_popup {
class XdgPopup : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(XdgPopup);
  XdgPopup(wl_resource*                         resource,
      std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
      std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
      xdg_positioner::XdgPositioner*            positioner);
  ~XdgPopup() override;

  void                                          commit() override;
  [[nodiscard]] std::shared_ptr<core::Toplevel> get_toplevel() override;

  void replace_positioner(xdg_positioner::XdgPositioner* positioner);
  /// @param token pass value if it is called via protocol,
  /// otherwise (e.g. initialization) pass std::nullopt
  void reposition(std::optional<uint32_t> token);
  void start_grab();

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

  enum class State : uint8_t {
    INITIAL,
    COMMITTED,
    CONFIGURED,
  } state_ = State::INITIAL;
};

void create(wl_client* client, int version, uint32_t id,
    std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
    std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
    xdg_positioner::XdgPositioner*            positioner);
}  // namespace yabe::proto::xdg::shell::xdg_popup
