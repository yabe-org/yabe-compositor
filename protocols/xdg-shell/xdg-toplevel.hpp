#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

namespace yabe::proto::xdg_shell::xdg_toplevel {
wl_resource* create(wl_client* client, uint32_t id);
}
