#pragma once

#include <wayland-server-core.h>

#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
void create(wl_client* client, uint32_t id, core::Surface* surface);
}
