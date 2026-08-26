#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::workspace {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
