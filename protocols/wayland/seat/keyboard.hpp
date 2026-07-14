#pragma once

#include <wayland-server.h>

namespace yabe::proto::wayland::seat::keyboard {
void create(wl_client* client, uint32_t id);
}
