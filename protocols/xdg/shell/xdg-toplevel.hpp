#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"
#include "core/scene/decoration-node.hpp"
#include "core/scene/resize-handle-node.hpp"
#include "core/scene/surface-node.hpp"
#include "core/surface.hpp"
#include "core/toplevel.hpp"
#include "protocols/xdg/shell/xdg-surface.hpp"

namespace yabe::proto::xdg::shell::xdg_toplevel {
class XdgToplevel : public core::SurfaceRole,
                    public core::Toplevel,
                    public std::enable_shared_from_this<XdgToplevel> {
 public:
  DISABLE_MOVE_AND_COPY(XdgToplevel);
  XdgToplevel(
      wl_resource* resource, std::weak_ptr<xdg_surface::XdgSurface>&& surface);
  ~XdgToplevel() override;

  void                                          commit() override;
  [[nodiscard]] std::shared_ptr<core::Toplevel> get_toplevel() override;

  void map();
  void set_activated(bool is_activated) override;
  [[nodiscard]] std::weak_ptr<core::Surface> surface() override;

  void start_move();
  void start_resize(uint32_t edges);
  void request_resize(common::unit::Pixel width, common::unit::Pixel height);

  [[nodiscard]] std::weak_ptr<xdg_surface::XdgSurface> xdg_surface() const {
    return this->xdg_surface_;
  }
  [[nodiscard]] std::weak_ptr<core::scene::SurfaceNode> surface_node();

 private:
  void send_configure(int32_t width, int32_t height);

  wl_resource* resource_;

  std::shared_ptr<core::scene::DecorationNode>   decoration_;
  std::shared_ptr<core::scene::ResizeHandleNode> resize_handle_;
  std::weak_ptr<xdg_surface::XdgSurface>         xdg_surface_;

  bool is_initial_commit_sent_ = false;
  bool is_activated_           = false;
};

wl_resource* create(wl_client* client, int version, uint32_t id,
    std::weak_ptr<xdg_surface::XdgSurface> xdg_surface);
}  // namespace yabe::proto::xdg::shell::xdg_toplevel
