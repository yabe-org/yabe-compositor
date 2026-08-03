#include "yabe-compositor.h"

#include <unistd.h>

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
}

void yabe_compositor_move_pointer(float dx, float dy) {
  yabe::core::server::get().enqueue([dx, dy]() {
    yabe::core::server::get().seat.pointer.handle_pointer_move_relative(dx, dy);
  });
}
void yabe_compositor_button(uint32_t button, bool is_pressed) {
  yabe::core::server::get().enqueue([button, is_pressed]() {
    yabe::core::server::get().seat.pointer.handle_pointer_button(
        button, is_pressed);
  });
}
void yabe_compositor_axis(uint32_t axis, float value) {
  yabe::core::server::get().enqueue([axis, value]() {
    yabe::core::server::get().seat.pointer.handle_pointer_axis(axis, value);
  });
}

void yabe_compositor_keymap(uint32_t format, int32_t fd, uint32_t size) {
  const int duplicated_fd = fd >= 0 ? dup(fd) : -1;
  yabe::core::server::get().enqueue([format, duplicated_fd, size]() {
    yabe::core::server::get().seat.keyboard.set_keymap(
        format, duplicated_fd, size);
    if (duplicated_fd >= 0) {
      close(duplicated_fd);
    }
  });
}
void yabe_compositor_repeat_info(int32_t rate, int32_t delay) {
  yabe::core::server::get().enqueue([rate, delay]() {
    yabe::core::server::get().seat.keyboard.set_repeat_info(rate, delay);
  });
}
void yabe_compositor_key(uint32_t key, bool is_pressed) {
  yabe::core::server::get().enqueue([key, is_pressed]() {
    yabe::core::server::get().seat.keyboard.handle_keyboard_key(
        key, is_pressed);
  });
}
void yabe_compositor_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group) {
  yabe::core::server::get().enqueue(
      [mods_depressed, mods_latched, mods_locked, group]() {
        yabe::core::server::get().seat.keyboard.set_modifiers(
            mods_depressed, mods_latched, mods_locked, group);
      });
}
}
