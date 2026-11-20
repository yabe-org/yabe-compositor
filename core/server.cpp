#include "server.hpp"

#include <GLES3/gl32.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wayland-server-core.h>

#include <cmath>
#include <glm/glm.hpp>
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
  this->is_initialized_.store(true);
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
    const auto load = this->rendering_mutex_.lock();
    if (!this->is_initialized_.load()) {
      throw std::runtime_error("Server is not initialized");
    }
  }
  {
    const auto lock   = this->is_started_mutex_.lock();
    this->is_started_ = true;
  }
  this->enqueue_wayland_thread_task([this]() {
    this->setting.events.emit<setting::fields::Event::kStartup>();
  });
  wl_display_run(this->wl_display_);
  {
    const auto lock   = this->is_started_mutex_.lock();
    this->is_started_ = false;
  }

  const auto load = this->rendering_mutex_.lock();
  this->is_initialized_.store(false);
  wl_display_destroy_clients(this->wl_display_);
  wl_event_source_remove(this->task_source_);
  this->task_source_ = nullptr;
  close(this->task_dispatch_event_fd_);
  this->task_dispatch_event_fd_ = -1;
  wl_display_destroy(this->wl_display_);
  this->wl_display_ = nullptr;
  this->should_invalidate_graphics_.store(true);
}

void Server::load_config(const char* config_contents) {
  const auto lock = this->is_started_mutex_.lock();
  // before starting server, always load config
  if (!this->is_started_) {
    this->setting.load_config(config_contents);
    return;
  }
  // after starting, reload only occurs if auto reload is enabled
  if (!this->setting.config.get<"auto_reload_config">()) {
    return;
  }
  this->enqueue_wayland_thread_task(
      [this, config = std::string(config_contents)]() {
        this->setting.load_config(config.c_str());
        this->setting.events.emit<setting::fields::Event::kConfigReloaded>();
      });
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
    const std::scoped_lock lock(this->wayland_thread_tasks_.mutex);
    this->wayland_thread_tasks_.tasks.push_back(std::move(task));
  }
  const uint64_t value = 1;
  if (write(this->task_dispatch_event_fd_, &value, sizeof(value)) < 0 &&
      errno != EAGAIN) {
    LOG_ERR("Failed to wake compositor event loop: {}", strerror(errno));
  }
}
void Server::enqueue_rendering_thread_task(std::function<void()> task) {
  const std::scoped_lock lock(this->rendering_thread_tasks_.mutex);
  this->rendering_thread_tasks_.tasks.push_back(std::move(task));
}

void Server::draw(const float* proj_mat, const float* view_mat) {
  const auto rendering_lock = this->rendering_mutex_.lock();
  if (!this->is_initialized_.load()) {
    return;
  }
  if (this->should_invalidate_graphics_.load()) {
    {
      const std::scoped_lock lock(this->rendering_thread_tasks_.mutex);
      this->rendering_thread_tasks_.tasks.clear();
    }
    this->rect_renderer.invalidate();
    this->resize_handle_renderer.invalidate();
    this->ray_renderer_.invalidate();
    this->ray_cursor.invalidate_graphics();
    this->is_first_rendering_ = true;
    this->should_invalidate_graphics_.store(false);
  }
  this->thread_for_rendering_thread_tasks_    = std::thread([this]() {
    this->rendering_thread_tasks_.consume();
  });
  const GLboolean was_cull_face_enabled       = glIsEnabled(GL_CULL_FACE);
  const GLboolean was_depth_test_enabled      = glIsEnabled(GL_DEPTH_TEST);
  const GLboolean was_blend_enabled           = glIsEnabled(GL_BLEND);
  GLint           previous_program            = 0;
  GLint           previous_vao                = 0;
  GLint           previous_array_buffer       = 0;
  GLint           previous_active_texture     = 0;
  GLint           previous_texture_2d         = 0;
  GLint           previous_unpack_alignment   = 0;
  GLint           previous_unpack_row_length  = 0;
  GLint           previous_unpack_skip_pixels = 0;
  GLint           previous_unpack_skip_rows   = 0;
  GLint           previous_blend_src_rgb      = 0;
  GLint           previous_blend_dst_rgb      = 0;
  GLint           previous_blend_src_alpha    = 0;
  GLint           previous_blend_dst_alpha    = 0;
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

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  this->view_mat_ = glm::make_mat4(view_mat) * this->anchor_mat_;
  this->proj_mat_ = glm::make_mat4(proj_mat);
  this->draw_content();

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
  proto::wayland::output::set_viewport_size(viewport.at(2), viewport.at(3));
}

bool Server::update_anchor_mat() {
  const auto      lock     = this->rendering_mutex_.lock();
  // pose of the head (eye) in the tracking space
  const glm::mat4 head_mat = glm::inverse(this->view_mat_);
  const glm::vec3 position = glm::vec3(glm::column(head_mat, 3));
  const glm::vec3 forward  = -glm::vec3(glm::column(head_mat, 2));
  // only yaw is reset to keep the direction of gravity
  if (glm::length(glm::vec2(forward.x, forward.z)) < 1e-3F) {
    LOG_WARN("Cannot recenter while looking straight up or down");
    return false;
  }
  const float yaw  = std::atan2(-forward.x, -forward.z);
  const auto delta = glm::translate(glm::mat4(1.F), position) *
                     glm::rotate(glm::mat4(1.F), yaw, glm::vec3(0.F, 1.F, 0.F));
  this->anchor_mat_ = this->anchor_mat_ * delta;
  this->view_mat_   = this->view_mat_ * delta;
  this->layer_manager.reset_rotation();
  return true;
}

void Server::draw_content() {
  const auto vp_mat = this->proj_mat_ * this->view_mat_;
  this->layer_manager.update_rotation(this->view_mat_);
  this->layer_manager.draw_under_scene(this->proj_mat_, this->view_mat_);
  this->workspace.current_scene().draw(this->proj_mat_, this->view_mat_);
  this->seat.input_method.draw(vp_mat);
  this->layer_manager.draw_above_scene(this->proj_mat_, this->view_mat_);
  server_seat::pointer_grab::ToplevelSelectionPointerGrab::draw(
      this->lasso_renderer_, vp_mat);
  this->ray_cursor.draw_cursor(vp_mat);
  this->ray_renderer_.render(vp_mat, this->ray_cursor.origin(),
      this->ray_cursor.direction(), this->ray_cursor.length());
}

uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}
glm::mat4 Server::view_mat() {
  const auto lock = this->rendering_mutex_.lock();
  return this->view_mat_;
}
glm::mat4 Server::proj_mat() {
  const auto lock = this->rendering_mutex_.lock();
  return this->proj_mat_;
}
glm::vec3 Server::head_direction() {
  const auto head_mat = glm::inverse(this->view_mat());
  return -glm::vec3(glm::column(head_mat, 2));
}
}  // namespace yabe::core::server
