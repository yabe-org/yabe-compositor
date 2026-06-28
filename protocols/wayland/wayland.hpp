#pragma once

#include <wayland-server-core.h>

namespace yabe::proto::wayland {
bool init(wl_display* display);
}
