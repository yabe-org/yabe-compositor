#pragma once

#include <wayland-server.h>

#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common/common.hpp"
#include "core/layer-manager.hpp"
#include "core/ray-cursor.hpp"
#include "core/render/ray-renderer.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/resource-storage.hpp"
#include "core/scene/scene.hpp"
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
  void     terminate();
  void     add_client_fd(int fd);
  void     enqueue_wayland_thread_task(std::function<void()> task);
  void     enqueue_rendering_thread_task(std::function<void()> task);
  void     draw(const float* proj_mat, const float* view_mat);

  uint32_t       next_serial();
  wl_event_loop* loop();

  ResourceStorage         resources;
  setting::Setting        setting;
  server_seat::ServerSeat seat;
  RayCursor               ray_cursor;
  Workspace               workspace;
  LayerManager            layer_manager;

  render::RectRenderer rect_renderer;

  std::unordered_map<wl_client*, wl_resource*> output_map;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  void draw_content(const glm::mat4& proj_mat, const glm::mat4& view_mat);

  bool is_initialized_     = false;
  bool is_first_rendering_ = true;

  render::RayRenderer ray_renderer_;
  wl_display*         wl_display_             = nullptr;
  wl_event_source*    task_source_            = nullptr;
  int                 task_dispatch_event_fd_ = -1;
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
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}

}  // namespace yabe::core::server
