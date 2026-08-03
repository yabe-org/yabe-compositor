#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"
#include "core/scene/toplevel.hpp"
#include "core/surface.hpp"
#include "protocols/xdg-shell/xdg-surface.hpp"

namespace yabe::proto::xdg_shell::xdg_toplevel {
class XdgToplevel : public core::SurfaceRole,
                    public std::enable_shared_from_this<XdgToplevel> {
 public:
  DISABLE_MOVE_AND_COPY(XdgToplevel);
  XdgToplevel(wl_resource*                     resource,
      std::weak_ptr<xdg_surface::XdgSurface>&& surface,
      std::weak_ptr<core::scene::Toplevel>&&   toplevel);
  ~XdgToplevel() override = default;

  void commit() override;
  void set_activated(bool is_activated) override;

  void start_move();
  void start_resize(uint32_t edges);
  void request_resize(core::unit::Pixel width, core::unit::Pixel height);

  [[nodiscard]] std::weak_ptr<core::scene::Toplevel> scene_node() const {
    return this->toplevel_;
  }
  [[nodiscard]] std::weak_ptr<xdg_surface::XdgSurface> xdg_surface() const {
    return this->xdg_surface_;
  }

 private:
  void send_configure(int32_t width, int32_t height);

  wl_resource*                           resource_;
  std::weak_ptr<xdg_surface::XdgSurface> xdg_surface_;
  std::weak_ptr<core::scene::Toplevel>   toplevel_;
  bool                                   is_initial_commit_sent_ = false;
  bool                                   is_activated_           = false;
};

wl_resource* create(wl_client* client, uint32_t id,
    std::weak_ptr<xdg_surface::XdgSurface> xdg_surface);
}  // namespace yabe::proto::xdg_shell::xdg_toplevel
