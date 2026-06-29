#include "yabe-compositor.h"

#include <exception>

#include "common/common.hpp"
#include "core/server.hpp"

extern "C" {
void yabe_compositor_start(void (*print_log)(const char*), int fd) {
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  LOG_INFO("yabe_compositor_start() called with fd: {}", fd);
  try {
    yabe::core::server::Server::get_instance().start(fd);
  } catch (std::exception& e) {
    LOG_ERR("Exception in yabe_compositor_start: {}", e.what());
  }
}

void yabe_compositor_terminate() {
  yabe::core::server::Server::get_instance().terminate();
}

void yabe_compositor_draw(const float* vp_mat) {
  yabe::core::server::Server::get_instance().draw(vp_mat);
}
}
