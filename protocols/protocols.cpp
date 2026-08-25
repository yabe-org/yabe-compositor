#include "protocols.hpp"

#include <wayland-server.h>

#include "wayland/wayland.hpp"
#include "wlr/wlr.hpp"
#include "xdg/xdg.hpp"
#include "zwin/zwin.hpp"

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
  ENSURE(wlr::init(display));
  ENSURE(xdg::init(display));
  ENSURE(zwin::init(display));

  return true;
#undef ENSURE
}
}  // namespace yabe::proto
