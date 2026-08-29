#include "yabe-compositor.h"

#include <unistd.h>

#include <exception>
#include <stdexcept>

#include "common/common.hpp"
#include "core/server.hpp"

extern "C" {
void yabe_compositor_init(void (*print_log)(const char*),
    const char* (*create_keymap)(
        const char*, const char*, const char*, const char*, const char*),
    int (*execvp)(const char* const *)) {
  if (create_keymap == nullptr) {
    throw std::invalid_argument("create_keymap function pointer is null");
  }
  if (execvp == nullptr) {
    throw std::invalid_argument("execvp function pointer cannot be null");
  }
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  try {
    yabe::core::server::get().seat.keyboard.set_create_keymap_func(
        create_keymap);
    yabe::core::server::get().setting.dispatchers.set_executer(execvp);
    yabe::core::server::get().init();
  } catch (std::exception& e) {
    LOG_ERR("Exception in yabe_compositor_init: {}", e.what());
  }
}

void yabe_compositor_load_config(const char* config_contents) {
  yabe::core::server::get().setting.load_config(config_contents);
}

uint32_t yabe_compositor_register_socket_fd(int fd) {
  auto fd_opt = (fd == -1) ? std::nullopt : std::optional<int>(fd);
  return yabe::core::server::get().set_socket_fd(fd_opt);
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

void yabe_compositor_draw(const float* proj_mat, const float* view_mat) {
  yabe::core::server::get().draw(proj_mat, view_mat);
}

void yabe_compositor_move_pointer(float dx, float dy) {
  yabe::core::server::get().enqueue_wayland_thread_task([dx, dy]() {
    const auto multiplier = static_cast<float>(yabe::core::server::get()
            .setting.config.get<"input", "pointer", "movement_multiplier">());
    yabe::core::server::get().seat.pointer.handle_pointer_move_relative(
        dx * multiplier, dy * multiplier);
  });
}
void yabe_compositor_button(uint32_t button, bool is_pressed) {
  yabe::core::server::get().enqueue_wayland_thread_task([button, is_pressed]() {
    yabe::core::server::get().seat.pointer.handle_pointer_button(
        button, is_pressed);
  });
}
void yabe_compositor_axis(uint32_t axis, float value) {
  yabe::core::server::get().enqueue_wayland_thread_task([axis, value]() {
    const auto multiplier = static_cast<float>(yabe::core::server::get()
            .setting.config.get<"input", "pointer", "scroll_multiplier">());
    yabe::core::server::get().seat.pointer.handle_pointer_axis(
        axis, value * multiplier);
  });
}

void yabe_compositor_key(uint32_t key, bool is_pressed) {
  yabe::core::server::get().enqueue_wayland_thread_task([key, is_pressed]() {
    yabe::core::server::get().seat.keyboard.handle_keyboard_key(
        key, is_pressed);
  });
}
}
