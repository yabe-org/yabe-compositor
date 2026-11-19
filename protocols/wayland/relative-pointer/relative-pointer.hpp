#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::relative_pointer {
void create(wl_client* client, int version, uint32_t id);
}
