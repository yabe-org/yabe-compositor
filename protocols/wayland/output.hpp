#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland::output {
struct ViewportSize {
  int32_t width;
  int32_t height;
};

void         set_viewport_size(int32_t width, int32_t height);
ViewportSize get_viewport_size();

void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}  // namespace yabe::proto::wayland::output
