#include "xdg.hpp"

#include <xdg-output-protocol.h>
#include <xdg-shell-protocol.h>

#include "common/common.hpp"
#include "output/xdg-output-manager.hpp"
#include "shell/xdg-wm-base.hpp"

namespace yabe::proto::xdg {
bool init(wl_display* display) {
  if (!wl_global_create(display, &xdg_wm_base_interface,
          xdg_wm_base_interface.version, nullptr, shell::bind)) {
    LOG_ERR("Failed to create global (xdg_wm_base)");
    return false;
  }
  if (!wl_global_create(display, &zxdg_output_manager_v1_interface,
          zxdg_output_manager_v1_interface.version, nullptr, output::bind)) {
    LOG_ERR("Failed to create global (xdg_output_manager)");
    return false;
  }
  return true;
}
}  // namespace yabe::proto::xdg
