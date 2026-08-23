#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::output {
void    set_viewport_size(int32_t width, int32_t height);
int32_t get_viewport_width();
int32_t get_viewport_height();

void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}  // namespace yabe::proto::wayland::output
