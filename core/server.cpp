#include "server.hpp"

#include <GLES3/gl3.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include "common/common.hpp"
#include "core/server-seat/server-keyboard.hpp"
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

  this->seat.keyboard.init_keymap("", "", "us", "", "");

  this->task_dispatch_event_fd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (this->task_dispatch_event_fd_ < 0) {
    throw std::runtime_error("Failed to create compositor task event fd");
  }
  this->task_source_ = wl_event_loop_add_fd(
      this->loop(), this->task_dispatch_event_fd_, WL_EVENT_READABLE,
      [](int fd, uint32_t, void*) {
        uint64_t value = 0;
        while (read(fd, &value, sizeof(value)) < 0 && errno == EINTR) {}
        server::get().consume_queued_tasks();
        return 0;
      },
      this);
  if (!this->task_source_) {
    close(this->task_dispatch_event_fd_);
    this->task_dispatch_event_fd_ = -1;
    throw std::runtime_error("Failed to add compositor task event source");
  }

  LOG_INFO("Server is initialized");
  this->is_initialized_ = true;
}

uint32_t Server::set_socket_fd(std::optional<int> fd) {
  if (fd.has_value()) {
    if (wl_display_add_socket_fd(this->wl_display_, *fd) == 0) {
      throw std::runtime_error("Failed to add socket fd to Wayland display");
    }
    return 0;
  }
  constexpr int kMaxDisplayId = 32;
  uint32_t      display_id    = 1;
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
  return display_id;
}

void Server::start() {
  if (!this->is_initialized_) {
    throw std::runtime_error("Server is not initialized");
  }
  wl_display_run(this->wl_display_);

  wl_display_destroy_clients(this->wl_display_);
  wl_event_source_remove(this->task_source_);
  this->task_source_ = nullptr;
  close(this->task_dispatch_event_fd_);
  this->task_dispatch_event_fd_ = -1;
  wl_display_destroy(this->wl_display_);
  this->wl_display_     = nullptr;
  this->is_initialized_ = false;
}

void Server::add_client_fd(int fd) {
  this->enqueue([this, fd]() {
    if (wl_client_create(this->wl_display_, fd) == nullptr) {
      LOG_ERR("Failed to create Wayland client for fd: {}", fd);
      close(fd);
    }
  });
}

void Server::terminate() {
  if (!this->wl_display_) {
    return;
  }
  this->enqueue([this]() {
    wl_display_terminate(this->wl_display_);
  });
}

void Server::enqueue(std::function<void()> task) {
  {
    const std::lock_guard lock(this->task_mutex_);
    this->tasks_.push_back(std::move(task));
  }
  const uint64_t value = 1;
  if (write(this->task_dispatch_event_fd_, &value, sizeof(value)) < 0 &&
      errno != EAGAIN) {
    LOG_ERR("Failed to wake compositor event loop: {}", strerror(errno));
  }
}

void Server::consume_queued_tasks() {
  std::vector<std::function<void()>> tasks;
  {
    const std::lock_guard lock(this->task_mutex_);
    tasks.swap(this->tasks_);
  }
  for (auto& task : tasks) {
    task();
  }
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
  glGetIntegerv(GL_BLEND_SRC_RGB, &previous_blend_src_rgb);
  glGetIntegerv(GL_BLEND_DST_RGB, &previous_blend_dst_rgb);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &previous_blend_src_alpha);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &previous_blend_dst_alpha);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color.data());

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

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
        this->scene.draw(draw_rect);

        const auto drag_and_drop_icon_surface =
            this->ray_cursor.drag_and_drop_icon_surface().lock();
        if (drag_and_drop_icon_surface) {
          drag_and_drop_icon_surface->upload_texture_if_needed();
          draw_rect(this->ray_cursor.cursor_model_mat(),
              drag_and_drop_icon_surface->texture());
          drag_and_drop_icon_surface->finish_rendering();
        }

        const auto cursor_surface = this->ray_cursor.cursor_surface().lock();
        if (cursor_surface) {
          cursor_surface->upload_texture_if_needed();
          draw_rect(
              this->ray_cursor.cursor_model_mat(), cursor_surface->texture());
          cursor_surface->finish_rendering();
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
