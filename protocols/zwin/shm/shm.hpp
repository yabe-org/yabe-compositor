#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yabe::proto::zwin::shm {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
