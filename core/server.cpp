#include "server.hpp"

#include <GLES3/gl3.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "common/common.hpp"
#include "protocols/protocols.hpp"

namespace yabe::core::server {
Server Server::instance;  // NOLINT

void Server::init() {
  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    throw std::runtime_error("Failed to create Wayland display");
  }
  wl_display_init_shm(this->wl_display_);

  if (!proto::init(this->wl_display_)) {
    throw std::runtime_error("Failed to initialize Wayland protocols");
  }

  LOG_INFO("Server is initialized");
  this->is_initialized_ = true;
}

void Server::set_socket_fd(std::optional<int> fd) {
  if (fd.has_value()) {
    if (wl_display_add_socket_fd(this->wl_display_, *fd) == 0) {
      throw std::runtime_error("Failed to add socket fd to Wayland display");
    }
    return;
  }
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
}

void Server::start() {
  if (!this->is_initialized_) {
    throw std::runtime_error("Server is not initialized");
  }
  wl_display_run(this->wl_display_);
}

void Server::add_client_fd(int fd) {
  if (wl_client_create(this->wl_display_, fd) == nullptr) {
    LOG_ERR("Failed to create Wayland client for fd: {}", fd);
    close(fd);
  }
}

void Server::terminate() {
  if (this->wl_display_) {
    wl_display_destroy_clients(this->wl_display_);
    wl_display_terminate(this->wl_display_);
    wl_display_destroy(this->wl_display_);
    this->wl_display_ = nullptr;
  }
}

void Server::flush_clients() {
  wl_display_flush_clients(this->wl_display_);
}

void Server::draw(const float* vp_mat) {
  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0.F, 0.F, 0.F, 1.F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  this->rect_renderer_.rendering_scope(
      vp_mat, [this](render::DrawRectFunc draw_rect) {
        this->surfaces.foreach ([&draw_rect](Surface& surface) {
          // FIXME: temporary workaround
          if (surface.type() != SurfaceType::CURSOR) {
            surface.upload_texture_if_needed();
            draw_rect(surface.model_mat(), surface.texture());
            surface.send_frame_done();
          }
        });
        const auto cursor_surface =
            this->seat.pointer.get_cursor_surface().lock();
        if (cursor_surface) {
          glDisable(GL_DEPTH_TEST);
          cursor_surface->upload_texture_if_needed();
          draw_rect(cursor_surface->model_mat(), cursor_surface->texture());
          cursor_surface->send_frame_done();
          glEnable(GL_DEPTH_TEST);
        }
      });
  this->ray_renderer_.draw(vp_mat, this->ray_cursor.origin(),
      this->ray_cursor.direction(), this->ray_cursor.length());
}

uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}
}  // namespace yabe::core::server
