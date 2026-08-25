#include "zwin.hpp"

#include <zwin-gles-v32-protocol.h>
#include <zwin-protocol.h>
#include <zwin-shell-protocol.h>

#include "common/common.hpp"
#include "compositor.hpp"
#include "gles-v32/gles-v32.hpp"
#include "seat.hpp"
#include "shell/shell.hpp"
#include "shm/shm.hpp"

namespace yabe::proto::zwin {
bool init(wl_display* display) {
  if (!wl_global_create(display, &zwn_shm_interface, zwn_shm_interface.version,
          nullptr, shm::bind)) {
    LOG_ERR("Failed to create global (zwn_shm)");
    return false;
  }
  if (!wl_global_create(display, &zwn_compositor_interface,
          zwn_compositor_interface.version, nullptr, compositor::bind)) {
    LOG_ERR("Failed to create global (zwn_compositor)");
    return false;
  }
  if (!wl_global_create(display, &zwn_seat_interface,
          zwn_seat_interface.version, nullptr, seat::bind)) {
    LOG_ERR("Failed to create global (zwn_seat)");
    return false;
  }
  if (!wl_global_create(display, &zwn_shell_interface,
          zwn_shell_interface.version, nullptr, shell::bind)) {
    LOG_ERR("Failed to create global (zwn_shell)");
    return false;
  }
  if (!wl_global_create(display, &zwn_gles_v32_interface,
          zwn_gles_v32_interface.version, nullptr, gles_v32::bind)) {
    LOG_ERR("Failed to create global (zwn_gles_v32)");
    return false;
  }
  return true;
}
}  // namespace yabe::proto::zwin
