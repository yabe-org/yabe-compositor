#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wlr::input_method {
void create_input_method(wl_client* client, int version, uint32_t id);
}
