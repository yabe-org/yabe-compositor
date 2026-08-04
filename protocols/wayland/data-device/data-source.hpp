#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::data_device_manager::data_source {
void create(wl_client* client, uint32_t id);
}
