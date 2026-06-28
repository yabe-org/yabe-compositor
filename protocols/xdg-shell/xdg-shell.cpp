#include "xdg-shell.hpp"

#include <xdg-shell-protocol.h>

#include "common/common.hpp"
#include "xdg-wm-base.hpp"

namespace yabe::proto::xdg_shell {
bool init(wl_display* display) {
  if (!wl_global_create(display, &xdg_wm_base_interface,
          xdg_wm_base_interface.version, nullptr, xdg_wm_base::bind)) {
    LOG_ERR("Failed to create global (xdg_wm_base)");
    return false;
  }

  return true;
}
}  // namespace yabe::proto::xdg_shell
