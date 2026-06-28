#include "common.hpp"

#include <cstdio>

namespace {
void default_print_log(const char* message) {
  std::puts(message);
}
}  // namespace

namespace yabe {
void (*print_log)(const char*) = default_print_log;  // NOLINT
}
