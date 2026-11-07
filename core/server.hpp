#pragma once

#include <wayland-server.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/common.hpp"
#include "common/thread-safety.hpp"
#include "core/layer-manager.hpp"
#include "core/ray-cursor.hpp"
#include "core/render/lasso-renderer.hpp"
#include "core/render/ray-renderer.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/render/resize-handle-renderer.hpp"
#include "core/resource-storage.hpp"
#include "core/server-seat/server-seat.hpp"
#include "core/setting/setting.hpp"
#include "core/workspace.hpp"

namespace yabe::core::server {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  static Server& get_instance() {
    return instance;
  }

  void     init();
  uint32_t set_socket_fd(std::optional<int> fd);
  void     start();
  /// loads the config immediately before start(),
  /// otherwise enqueues it to the Wayland thread
  void     load_config(const char* config_contents);
  void     terminate();
  void     add_client_fd(int fd);
  void     enqueue_wayland_thread_task(std::function<void()> task);
  void     enqueue_rendering_thread_task(std::function<void()> task);
  void     draw(const float* proj_mat, const float* view_mat);
  /// makes the current head direction the front at the next draw()
  void     request_recenter();

  uint32_t                next_serial();
  wl_event_loop*          loop();
  [[nodiscard]] glm::mat4 view_mat();
  [[nodiscard]] glm::mat4 proj_mat();
  [[nodiscard]] glm::vec3 head_direction();

  ResourceStorage         resources;
  setting::Setting        setting;
  server_seat::ServerSeat seat;
  RayCursor               ray_cursor;
  Workspace               workspace;
  LayerManager            layer_manager;

  render::RectRenderer         rect_renderer;
  render::ResizeHandleRenderer resize_handle_renderer;

  std::unordered_map<wl_client*, wl_resource*> output_map;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  void draw_content(const glm::mat4& proj_mat, const glm::mat4& view_mat);
  /// @return false if it cannot recenter
  bool update_anchor_mat(const glm::mat4& view_mat);

  bool       is_initialized_             = false;
  bool       is_first_rendering_         = true;
  // some OpenXR platform (e.g. Meta Quest) does not fully stop the app process
  // while it releases OpenGL context. So, after being called terminate(),
  // we should invalidate OpenGL resources and re-create them
  // when the process is resumed
  bool       should_invalidate_graphics_ = false;
  std::mutex rendering_mutex_;

  common::Mutex    is_started_mutex_;
  bool is_started_ GUARDED_BY(is_started_mutex_) = false;

  render::RayRenderer   ray_renderer_;
  render::LassoRenderer lasso_renderer_;
  wl_display*           wl_display_             = nullptr;
  wl_event_source*      task_source_            = nullptr;
  int                   task_dispatch_event_fd_ = -1;
  struct {
    std::mutex                         mutex;
    std::vector<std::function<void()>> tasks;
    void                               consume() {
      std::vector<std::function<void()>> tasks;
      {
        const std::lock_guard lock(this->mutex);
        tasks.swap(this->tasks);
      }
      for (auto& task : tasks) {
        task();
      }
    }
  } wayland_thread_tasks_, rendering_thread_tasks_;
  std::thread thread_for_rendering_thread_tasks_;

  // (view|proj)_mat are only written at draw()
  common::Mutex       vp_mat_mutex_;
  glm::mat4 view_mat_ GUARDED_BY(vp_mat_mutex_) = glm::mat4(1.F);
  glm::mat4 proj_mat_ GUARDED_BY(vp_mat_mutex_) = glm::mat4(1.F);

  // transformation from the compositor world to the tracking space of the
  // host; only accessed in draw()
  glm::mat4         anchor_mat_ = glm::mat4(1.F);
  std::atomic<bool> is_recenter_requested_{false};
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}

}  // namespace yabe::core::server
