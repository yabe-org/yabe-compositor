#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"
#include "core/scene/toplevel.hpp"
#include "core/surface.hpp"
#include "protocols/xdg-shell/xdg-surface.hpp"

namespace yabe::proto::xdg_shell::xdg_toplevel {
class XdgToplevel : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(XdgToplevel);
  XdgToplevel(wl_resource*                     resource,
      std::weak_ptr<xdg_surface::XdgSurface>&& surface,
      std::weak_ptr<core::scene::Toplevel>&&   toplevel);
  ~XdgToplevel() override = default;

  void commit() override;
  void set_activated(bool is_activated) override;

  void move();

  [[nodiscard]] std::weak_ptr<core::scene::Toplevel> scene_node() const {
    return this->toplevel_;
  }

 private:
  wl_resource*                           resource_;
  std::weak_ptr<xdg_surface::XdgSurface> xdg_surface_;
  std::weak_ptr<core::scene::Toplevel>   toplevel_;
  bool                                   is_initial_commit_sent_ = false;
};

wl_resource* create(wl_client* client, uint32_t id,
    std::weak_ptr<xdg_surface::XdgSurface> xdg_surface);
}  // namespace yabe::proto::xdg_shell::xdg_toplevel
