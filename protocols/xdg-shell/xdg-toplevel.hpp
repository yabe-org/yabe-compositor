#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_toplevel {
wl_resource* create(
    wl_client* client, uint32_t id, std::weak_ptr<core::Surface>&& surface);

class XdgToplevel {
 public:
  DISABLE_MOVE_AND_COPY(XdgToplevel);
  explicit XdgToplevel(std::weak_ptr<core::Surface>&& surface);
  ~XdgToplevel() = default;

  void move();

 private:
  std::weak_ptr<core::Surface> surface_;
};
}  // namespace yabe::proto::xdg_shell::xdg_toplevel
