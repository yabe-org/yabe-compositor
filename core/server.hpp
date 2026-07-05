#pragma once

#include <wayland-server.h>

#include "common/common.hpp"
#include "core/ray-cursor.hpp"
#include "core/render/ray-renderer.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/server-seat/server-seat.hpp"
#include "core/surface-container.hpp"

namespace yabe::core::server {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  static Server& get_instance() {
    return instance;
  }

  void start(int fd);
  void terminate();
  void flush_clients();
  void draw(const float* vp_mat);

  uint32_t       next_serial();
  wl_event_loop* loop();

  SurfaceContainer        surfaces;
  server_seat::ServerSeat seat;
  RayCursor               ray_cursor;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  render::RectRenderer rect_renderer_;
  render::RayRenderer  ray_renderer_;
  wl_display*          wl_display_ = nullptr;
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}

}  // namespace yabe::core::server
