#include "yabe-compositor.h"

#include "common/common.hpp"

extern "C" {
void start(void (*print_log)(const char*), int fd) {
  if (print_log != nullptr) {
    yabe::print_log = print_log;
  }
  LOG_DBG("fd: {}", fd);
}

void draw() {
  // todo
}
}
