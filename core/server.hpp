#pragma once

#include <wayland-server.h>

#include "common/common.hpp"
#include "core/surface_container.hpp"

namespace yabe::core::server {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  static Server& get_instance() {
    return instance;
  }

  void start(int fd);
  void terminate();

  uint32_t       next_serial();
  wl_event_loop* loop();

  SurfaceContainer surfaces;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  wl_display* wl_display_ = nullptr;
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}

}  // namespace yabe::core::server
