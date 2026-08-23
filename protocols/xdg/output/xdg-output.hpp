#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::xdg::output::xdg_output {
void create(wl_client* client, int version, uint32_t id);
}
