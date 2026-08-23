#include "wlr.hpp"

#include <wlr-layer-shell-protocol.h>

#include "common/common.hpp"
#include "layer-shell.hpp"

namespace yabe::proto::wlr {
bool init(wl_display* display) {
  if (!wl_global_create(display, &zwlr_layer_shell_v1_interface,
          zwlr_layer_shell_v1_interface.version, nullptr, layer_shell::bind)) {
    LOG_ERR("Failed to create global (wlr_layer_shell)");
    return false;
  }

  return true;
}
}  // namespace yabe::proto::wlr
