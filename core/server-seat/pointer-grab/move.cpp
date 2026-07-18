#include "move.hpp"

#include <linux/input-event-codes.h>

#include <memory>

#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::core::server_seat::pointer_grab {
MovePointerGrab* MovePointerGrab::start(std::weak_ptr<Surface>&& surface) {
  static MovePointerGrab instance;
  server::get().seat.pointer.unfocus_surface();
  instance.moving_surface_ = std::move(surface);
  return &instance;
}

void MovePointerGrab::cleanup() {
  server::get().seat.pointer.focus_pointed_surface();
}

void MovePointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(-dy, -dx);

  const auto surface = this->moving_surface_.lock();
  if (!surface) {
    server::get().seat.pointer.end_grab();
    return;
  }
  surface->move_relative(dy, dx);
}

void MovePointerGrab::button(uint32_t button, bool is_pressed) {
  if (button == BTN_LEFT && !is_pressed) {
    server::get().seat.pointer.end_grab();
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
