#pragma once

#include <wayland-server.h>

namespace yabe::proto::wayland::surface {
void create(wl_client* client, uint32_t id);
}  // namespace yabe::proto::wayland::surface
