#include "server.hpp"

#include <wayland-server-core.h>

#include "protocols/protocols.hpp"

namespace yabe::core::server {
Server Server::instance;

void Server::start(int fd) {
  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    throw std::runtime_error("Failed to create Wayland display");
  }
  if (fd == -1) {
    if (!wl_display_add_socket_auto(this->wl_display_)) {
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

  wl_display_run(this->wl_display_);
}

void Server::terminate() {
  if (this->wl_display_) {
    wl_display_destroy_clients(this->wl_display_);
    wl_display_terminate(this->wl_display_);
    wl_display_destroy(this->wl_display_);
    this->wl_display_ = nullptr;
  }
}

uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}
}  // namespace yabe::core::server
