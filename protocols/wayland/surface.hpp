#pragma once

#include <wayland-server.h>

namespace yabe::proto::wayland::surface {
bool is_surface_resource(wl_resource* resource);
void create(wl_client* client, uint32_t id);
}  // namespace yabe::proto::wayland::surface
