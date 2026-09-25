#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wlr::virtual_keyboard {
void create(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::wlr::virtual_keyboard
