# yabe-compositor

A Wayland compositor that works as a library

## Interface

This library provides following functions:

- `void yabe_compositor_start(void (*print_log)(const char*), int fd)`
- `void yabe_compositor_terminate()`
- `void yabe_compositor_draw(const float* vp_mat);`

## Usage

Add this repository as a submodule to your project. Then write `CMakeLists.txt` like this:

```cmake
include(ExternalProject)
ExternalProject_Add(
    yabe_compositor_project
    SOURCE_DIR path/to/submodule
    CONFIGURE_COMMAND meson setup <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND meson compile -C <BINARY_DIR>
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS <BINARY_DIR>/libcompositor_core.a
)
ExternalProject_Get_Property(yabe_compositor BINARY_DIR)
set(MESON_BUILD_DIR ${BINARY_DIR})
# Define yabe_compositor as a IMPORTED target
add_library(yabe_compositor STATIC IMPORTED)
set_target_properties(yabe_compositor $PROPERTIES
    IMPORTED_LOCATION ${MESON_BUILD_DIR}/libyabe_compositor.a
)
add_dependencies(cmake-wayland-test meson_compositor)
```

Then you can use this library in your project like this:

```c
#include <yabe_compositor.h>
#include <print>
#include <thread>

namespace {
  void print_log(const char* log) {
    std::println("{}", log);
  }
}

int main() {
  // initialize OpenGL etc
  auto thread = std::thread([]() {
      start(print_log, -1);
  });
  while (true) {
    draw();
  }
  thread.join();
  return 0;
}
```

## LICENSE

Dual-licensed; MIT (`LICENSE-MIT` or [The MIT License – Open Source Initiative](https://opensource.org/license/mit/)) or MIT SUSHI-WARE LICENSE (`LICENSE-MIT_SUSHI.md`)
