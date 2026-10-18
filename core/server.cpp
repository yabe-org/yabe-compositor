#include "server.hpp"

#include <GLES3/gl32.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include <mutex>

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/toplevel-selection.hpp"
#include "core/server-seat/server-keyboard.hpp"
#include "protocols/protocols.hpp"
#include "protocols/wayland/output.hpp"

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
        server::get().wayland_thread_tasks_.consume();
        return 0;
      },
      this);
  if (!this->task_source_) {
    close(this->task_dispatch_event_fd_);
    this->task_dispatch_event_fd_ = -1;
    throw std::runtime_error("Failed to add compositor task event source");
  }

  LOG_INFO("Server is initialized");
  const std::lock_guard rendering_lock(this->rendering_mutex_);
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
  {
    const std::lock_guard rendering_lock(this->rendering_mutex_);
    if (!this->is_initialized_) {
      throw std::runtime_error("Server is not initialized");
    }
  }
  wl_display_run(this->wl_display_);

  const std::lock_guard rendering_lock(this->rendering_mutex_);
  this->is_initialized_ = false;
  wl_display_destroy_clients(this->wl_display_);
  wl_event_source_remove(this->task_source_);
  this->task_source_ = nullptr;
  close(this->task_dispatch_event_fd_);
  this->task_dispatch_event_fd_ = -1;
  wl_display_destroy(this->wl_display_);
  this->wl_display_                 = nullptr;
  this->should_invalidate_graphics_ = true;
}

void Server::add_client_fd(int fd) {
  this->enqueue_wayland_thread_task([this, fd]() {
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
  this->enqueue_wayland_thread_task([this]() {
    wl_display_terminate(this->wl_display_);
  });
}

void Server::enqueue_wayland_thread_task(std::function<void()> task) {
  {
    const std::lock_guard lock(this->wayland_thread_tasks_.mutex);
    this->wayland_thread_tasks_.tasks.push_back(std::move(task));
  }
  const uint64_t value = 1;
  if (write(this->task_dispatch_event_fd_, &value, sizeof(value)) < 0 &&
      errno != EAGAIN) {
    LOG_ERR("Failed to wake compositor event loop: {}", strerror(errno));
  }
}
void Server::enqueue_rendering_thread_task(std::function<void()> task) {
  const std::lock_guard lock(this->rendering_thread_tasks_.mutex);
  this->rendering_thread_tasks_.tasks.push_back(std::move(task));
}

void Server::draw(const float* proj_mat, const float* view_mat) {
  const std::lock_guard rendering_lock(this->rendering_mutex_);
  if (!this->is_initialized_) {
    return;
  }
  if (this->should_invalidate_graphics_) {
    {
      const std::lock_guard lock(this->rendering_thread_tasks_.mutex);
      this->rendering_thread_tasks_.tasks.clear();
    }
    this->rect_renderer.invalidate();
    this->resize_handle_renderer.invalidate();
    this->ray_renderer_.invalidate();
    this->ray_cursor.invalidate_graphics();
    this->is_first_rendering_         = true;
    this->should_invalidate_graphics_ = false;
  }
  this->thread_for_rendering_thread_tasks_         = std::thread([this]() {
    this->rendering_thread_tasks_.consume();
  });
  const GLboolean        was_cull_face_enabled     = glIsEnabled(GL_CULL_FACE);
  const GLboolean        was_depth_test_enabled    = glIsEnabled(GL_DEPTH_TEST);
  const GLboolean        was_blend_enabled         = glIsEnabled(GL_BLEND);
  GLint                  previous_program          = 0;
  GLint                  previous_vao              = 0;
  GLint                  previous_array_buffer     = 0;
  GLint                  previous_active_texture   = 0;
  GLint                  previous_texture_2d       = 0;
  GLint                  previous_unpack_alignment = 0;
  GLint                  previous_unpack_row_length  = 0;
  GLint                  previous_unpack_skip_pixels = 0;
  GLint                  previous_unpack_skip_rows   = 0;
  GLint                  previous_blend_src_rgb      = 0;
  GLint                  previous_blend_dst_rgb      = 0;
  GLint                  previous_blend_src_alpha    = 0;
  GLint                  previous_blend_dst_alpha    = 0;
  std::array<GLfloat, 4> previous_clear_color        = {};
  glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previous_vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
  glActiveTexture(GL_TEXTURE0);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture_2d);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &previous_unpack_row_length);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &previous_unpack_skip_pixels);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS, &previous_unpack_skip_rows);
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
  this->draw_content(glm::make_mat4(proj_mat), glm::make_mat4(view_mat));

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
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, previous_unpack_skip_pixels);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, previous_unpack_skip_rows);
  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
  glBindTexture(GL_TEXTURE_2D, previous_texture_2d);
  glActiveTexture(previous_active_texture);
  glBindBuffer(GL_ARRAY_BUFFER, previous_array_buffer);
  glBindVertexArray(previous_vao);
  glUseProgram(previous_program);

  if (this->thread_for_rendering_thread_tasks_.joinable()) {
    this->thread_for_rendering_thread_tasks_.join();
  }

  if (!this->is_first_rendering_) {
    return;
  }
  this->is_first_rendering_     = false;
  std::array<GLint, 4> viewport = {};  // x, y, width, height
  glGetIntegerv(GL_VIEWPORT, viewport.data());
  proto::wayland::output::set_viewport_size(viewport[2], viewport[3]);
}

void Server::draw_content(
    const glm::mat4& proj_mat, const glm::mat4& view_mat) {
  {
    const std::lock_guard lock(this->vp_mat_mutex_);
    this->vp_mat_ = proj_mat * view_mat;
  }
  this->layer_manager.draw_under_scene(proj_mat);
  this->workspace.current_scene().draw(proj_mat, view_mat);
  this->layer_manager.draw_above_scene(proj_mat);
  server_seat::pointer_grab::ToplevelSelectionPointerGrab::draw(
      this->lasso_renderer_, this->vp_mat_);
  this->ray_cursor.draw_cursor(this->vp_mat_);
  this->ray_renderer_.render(this->vp_mat_, this->ray_cursor.origin(),
      this->ray_cursor.direction(), this->ray_cursor.length());
}

uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}
glm::mat4 Server::get_vp_mat() {
  const std::lock_guard lock(this->vp_mat_mutex_);
  return this->vp_mat_;
}
}  // namespace yabe::core::server
