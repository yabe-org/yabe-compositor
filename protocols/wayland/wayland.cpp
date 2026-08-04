#include "wayland.hpp"

#include <wayland-server-protocol.h>

#include "common/common.hpp"
#include "compositor.hpp"
#include "data-device/data-device-manager.hpp"
#include "output.hpp"
#include "seat/seat.hpp"

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
  if (!wl_global_create(display, &wl_seat_interface, wl_seat_interface.version,
          nullptr, seat::bind)) {
    LOG_ERR("Failed to create global (seat)");
    return false;
  }
  if (!wl_global_create(display, &wl_data_device_manager_interface,
          wl_data_device_manager_interface.version, nullptr,
          data_device_manager::bind)) {
    LOG_ERR("Failed to create global (data_device_manager)");
    return false;
  }
  return true;
}
}  // namespace yabe::proto::wayland
