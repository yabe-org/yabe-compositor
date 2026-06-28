#include "yabe-compositor.h"

#include "common/common.hpp"
#include "core/server.hpp"

extern "C" {
void yabe_compositor_start(void (*print_log)(const char*), int fd) {
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  yabe::core::server::Server::get_instance().start(fd);
}

void yabe_compositor_terminate() {
  yabe::core::server::Server::get_instance().terminate();
}

void yabe_compositor_draw(const float* vp_mat) {
  yabe::core::server::Server::get_instance().draw(vp_mat);
}
}
