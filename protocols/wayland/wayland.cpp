#include "wayland.hpp"

#include <wayland-server-protocol.h>

#include "common/common.hpp"
#include "compositor.hpp"
#include "output.hpp"

namespace yabe::proto::wayland {
bool init(wl_display* display) {
  if (!wl_global_create(display, &wl_compositor_interface,
          wl_compositor_interface.version, nullptr, compositor::bind)) {
    LOG_ERR("Failed to create global (compositor)");
    return false;
  }
  if (!wl_global_create(display, &wl_output_interface,
          wl_output_interface.version, nullptr, output::bind)) {
    LOG_ERR("Failed to create global (output)");
    return false;
  }
  return true;
}
}  // namespace yabe::proto::wayland
