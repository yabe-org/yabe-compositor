#include "protocols.hpp"

#include <wayland-server.h>

#include "wayland/wayland.hpp"
#include "xdg-shell/xdg-shell.hpp"

namespace yabe::proto {
bool init(wl_display* display) {
  // NOLINTNEXTLINE: This cannot be replaced with constexpr function
#define ENSURE(call)                                                           \
  do {                                                                         \
    if (!(call)) {                                                             \
      return false;                                                            \
    }                                                                          \
  } while (false)

  ENSURE(wayland::init(display));
  ENSURE(xdg_shell::init(display));

  return true;
#undef ENSURE
}
}  // namespace yabe::proto
