#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::zwin::gles_v32 {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
