#pragma once

#include <wayland-server.h>

namespace yabe::proto::wayland::region {
void create(wl_client* client, uint32_t id);
}  // namespace yabe::proto::wayland::region
