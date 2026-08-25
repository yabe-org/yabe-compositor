#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yabe::proto::zwin::seat {
void create_ray(wl_client* client, int version, uint32_t id);
}
