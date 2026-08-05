#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::subcompositor::subsurface {
void create(wl_client* client, uint32_t id);
}
