# yabe-compositor

A Wayland compositor that works as a library


## Usage

Add this repository as a submodule to your project. Then write `CMakeLists.txt` like this:

```cmake
set(COMPOSITOR_SUBMODULE_DIR path/to/submodule)
include(ExternalProject)
ExternalProject_Add(
    yabe_compositor_project
    SOURCE_DIR ${COMPOSITOR_SUBMODULE_DIR}
    CONFIGURE_COMMAND meson setup <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND meson compile -C <BINARY_DIR>
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS <BINARY_DIR>/libyabe_compositor.a
)
ExternalProject_Get_Property(yabe_compositor BINARY_DIR)
set(MESON_BUILD_DIR ${BINARY_DIR})
add_library(yabe_compositor STATIC IMPORTED)
set_target_properties(yabe_compositor $PROPERTIES
    IMPORTED_LOCATION ${MESON_BUILD_DIR}/libyabe_compositor.a
)
add_dependencies(${PROJECT_NAME} meson_compositor)

target_include_directories(${PROJECT_NAME} PRIVATE ${MESON_BUILD_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE yabe_compositor)
```

Then you can use this library in your project like this:

```c
#include <yabe-compositor.h>
#include <array>
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
    yabe_compositor_init(print_log);
    yabe_compositor_register_socket_fd(-1);
    yabe_compositor_start();
  });
  while (true) {
    std::array<float, 16> vp_mat = /* update */;
    yabe_compositor_draw(vp_mat.data());
  }
  thread.join();
  return 0;
}
```

See `yabe-compositor.h` for details of the interface.

## LICENSE

Dual-licensed; MIT (`LICENSE-MIT` or [The MIT License – Open Source Initiative](https://opensource.org/license/mit/)) or MIT SUSHI-WARE LICENSE (`LICENSE-MIT_SUSHI.md`)
