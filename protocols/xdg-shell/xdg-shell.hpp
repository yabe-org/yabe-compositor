#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::xdg_shell {
bool init(wl_display* display);
}
