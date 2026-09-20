#include "wlr.hpp"

#include <input-method-protocol.h>
#include <wlr-layer-shell-protocol.h>

#include "common/common.hpp"
#include "input-method/input-method-manager.hpp"
#include "layer-shell.hpp"

namespace yabe::proto::wlr {
bool init(wl_display* display) {
  if (!wl_global_create(display, &zwlr_layer_shell_v1_interface,
          zwlr_layer_shell_v1_interface.version, nullptr, layer_shell::bind)) {
    LOG_ERR("Failed to create global (wlr_layer_shell)");
    return false;
  }
  if (!wl_global_create(display, &zwp_input_method_manager_v2_interface,
          zwp_input_method_manager_v2_interface.version, nullptr,
          input_method::bind)) {
    LOG_ERR("Failed to create global (input_method)");
    return false;
  }

  return true;
}
}  // namespace yabe::proto::wlr
