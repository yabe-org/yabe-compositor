#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::data_device_manager::data_device {
void create(wl_client* client, int version, uint32_t id);
}
