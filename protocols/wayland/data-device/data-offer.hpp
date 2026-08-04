#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::data_device_manager::data_offer {
void create(wl_client* client, uint32_t id);
}
