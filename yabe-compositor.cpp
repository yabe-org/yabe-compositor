#include "yabe-compositor.h"

#include <exception>

#include "common/common.hpp"
#include "core/server.hpp"
#include "wayland-client-core.h"

extern "C" {
void yabe_compositor_start(void (*print_log)(const char*), int fd) {
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  LOG_INFO("yabe_compositor_start() called with fd: {}", fd);
  try {
    yabe::core::server::get().start(fd);
  } catch (std::exception& e) {
    LOG_ERR("Exception in yabe_compositor_start: {}", e.what());
  }
}

void yabe_compositor_terminate() {
  yabe::core::server::get().terminate();
}

void yabe_compositor_draw(const float* vp_mat) {
  yabe::core::server::get().draw(vp_mat);
  // callbacks are sent during draw(), so we should call flush_clients()
  yabe::core::server::get().flush_clients();
}

void yabe_compositor_move_pointer(float dx, float dy) {
  yabe::core::server::get().seat.pointer.handle_pointer_move_relative(dx, dy);
  yabe::core::server::get().flush_clients();
}
void yabe_compositor_button(uint32_t button, bool is_pressed) {
  yabe::core::server::get().seat.pointer.handle_pointer_button(
      button, is_pressed);
  yabe::core::server::get().flush_clients();
}
void yabe_compositor_axis(uint32_t axis, float value) {
  yabe::core::server::get().seat.pointer.handle_pointer_axis(axis, value);
  yabe::core::server::get().flush_clients();
}
}
