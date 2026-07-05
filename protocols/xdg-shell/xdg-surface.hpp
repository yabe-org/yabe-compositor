#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::xdg_shell::xdg_surface {
void create(wl_client* client, uint32_t id, wl_resource* wl_surface);
}
