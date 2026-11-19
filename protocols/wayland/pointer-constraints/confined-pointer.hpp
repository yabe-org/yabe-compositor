#pragma once

#include <wayland-server-core.h>

#include "core/surface.hpp"

namespace yabe::proto::wayland::pointer_constraints::confined {
void create(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<core::Surface>& surface);
}
