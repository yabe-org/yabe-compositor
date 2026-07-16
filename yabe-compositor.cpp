#include "yabe-compositor.h"

#include <exception>

#include "common/common.hpp"
#include "core/server.hpp"

extern "C" {
void yabe_compositor_init(void (*print_log)(const char*)) {
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  try {
    yabe::core::server::get().init();
  } catch (std::exception& e) {
    LOG_ERR("Exception in yabe_compositor_init: {}", e.what());
  }
}

void yabe_compositor_register_socket_fd(int fd) {
  auto fd_opt = (fd == -1) ? std::nullopt : std::optional<int>(fd);
  yabe::core::server::get().set_socket_fd(fd_opt);
}

void yabe_compositor_start() {
  try {
    yabe::core::server::get().start();
  } catch (std::exception& e) {
    LOG_ERR("Exception in yabe_compositor_start: {}", e.what());
  }
}

void yabe_compositor_add_client_fd(int fd) {
  yabe::core::server::get().add_client_fd(fd);
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
