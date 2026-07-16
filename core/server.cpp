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
  const GLboolean        was_cull_face_enabled     = glIsEnabled(GL_CULL_FACE);
  const GLboolean        was_depth_test_enabled    = glIsEnabled(GL_DEPTH_TEST);
  const GLboolean        was_blend_enabled         = glIsEnabled(GL_BLEND);
  GLint                  previous_program          = 0;
  GLint                  previous_vao              = 0;
  GLint                  previous_array_buffer     = 0;
  GLint                  previous_active_texture   = 0;
  GLint                  previous_texture_2d       = 0;
  GLint                  previous_unpack_alignment = 0;
  GLint                  previous_unpack_row_length = 0;
  GLint                  previous_depth_func        = 0;
  GLint                  previous_blend_src_rgb     = 0;
  GLint                  previous_blend_dst_rgb     = 0;
  GLint                  previous_blend_src_alpha   = 0;
  GLint                  previous_blend_dst_alpha   = 0;
  std::array<GLfloat, 4> previous_clear_color       = {};
  glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previous_vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture_2d);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &previous_unpack_row_length);
  glGetIntegerv(GL_DEPTH_FUNC, &previous_depth_func);
  glGetIntegerv(GL_BLEND_SRC_RGB, &previous_blend_src_rgb);
  glGetIntegerv(GL_BLEND_DST_RGB, &previous_blend_dst_rgb);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &previous_blend_src_alpha);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &previous_blend_dst_alpha);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color.data());

  glDisable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(0.F, 0.F, 0.F, 1.F);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  this->draw_content(vp_mat);

  if (was_cull_face_enabled) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
  if (was_depth_test_enabled) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
  if (was_blend_enabled) {
    glEnable(GL_BLEND);
  } else {
    glDisable(GL_BLEND);
  }
  glDepthFunc(previous_depth_func);
  glBlendFuncSeparate(previous_blend_src_rgb, previous_blend_dst_rgb,
      previous_blend_src_alpha, previous_blend_dst_alpha);
  glClearColor(previous_clear_color[0], previous_clear_color[1],
      previous_clear_color[2], previous_clear_color[3]);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, previous_unpack_row_length);
  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
  glBindTexture(GL_TEXTURE_2D, previous_texture_2d);
  glActiveTexture(previous_active_texture);
  glBindBuffer(GL_ARRAY_BUFFER, previous_array_buffer);
  glBindVertexArray(previous_vao);
  glUseProgram(previous_program);
}

void Server::draw_content(const float* vp_mat) {
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
