#include "server.hpp"

#include <wayland-server-core.h>

#include "common/common.hpp"
#include "protocols/protocols.hpp"

namespace yabe::core::server {
Server Server::instance;

void Server::start(int fd) {
  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    throw std::runtime_error("Failed to create Wayland display");
  }
  if (fd == -1) {
    constexpr int kMaxDisplayId = 32;
    int           display_id    = 1;
    for (; display_id < kMaxDisplayId; ++display_id) {
      const auto socket_name = std::format("wayland-{}", display_id);
      if (wl_display_add_socket(this->wl_display_, socket_name.c_str()) == 0) {
        LOG_INFO("Wayland display socket created: {}", socket_name);
        break;
      }
    }
    if (display_id == kMaxDisplayId) {
      throw std::runtime_error("Failed to add socket to Wayland display");
    }
  } else {
    if (!wl_display_add_socket_fd(this->wl_display_, fd)) {
      throw std::runtime_error("Failed to add socket fd to Wayland display");
    }
  }
  wl_display_init_shm(this->wl_display_);

  if (!proto::init(this->wl_display_)) {
    throw std::runtime_error("Failed to initialize Wayland protocols");
  }

  LOG_INFO("Starting wl_display");
  wl_display_run(this->wl_display_);
  LOG_INFO("wl_display_run() finished");
}

void Server::terminate() {
  if (this->wl_display_) {
    wl_display_destroy_clients(this->wl_display_);
    wl_display_terminate(this->wl_display_);
    wl_display_destroy(this->wl_display_);
    this->wl_display_ = nullptr;
  }
}

void Server::draw(const float* vp_mat) {
  this->rect_renderer_.rendering_scope(
      vp_mat, [this](render::DrawRectFunc draw_rect) {
        this->surfaces.foreach ([&draw_rect](Surface& surface) {
          surface.upload_texture_if_needed();
          draw_rect(surface.model_mat(), surface.texture());
          surface.send_frame_done();
        });
      });
}

uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}
}  // namespace yabe::core::server
