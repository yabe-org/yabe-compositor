#pragma once

#include <wayland-server.h>

namespace yabe::proto::wayland::seat::pointer {
void create(wl_client* client, int version, uint32_t id);
}
