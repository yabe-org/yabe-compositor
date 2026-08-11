#pragma once

#include <wayland-server.h>

#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "common/common.hpp"
#include "core/data-device/data-device.hpp"
#include "core/data-device/data-offer.hpp"
#include "core/data-device/data-source.hpp"
#include "core/ray-cursor.hpp"
#include "core/render/ray-renderer.hpp"
#include "core/render/rect-renderer.hpp"
#include "core/scene/scene.hpp"
#include "core/server-seat/server-seat.hpp"
#include "core/surface.hpp"
#include "protocols/wayland/subcompositor/subsurface.hpp"
#include "protocols/xdg-shell/xdg-popup.hpp"
#include "protocols/xdg-shell/xdg-surface.hpp"
#include "protocols/xdg-shell/xdg-toplevel.hpp"

namespace yabe::core::server {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  static Server& get_instance() {
    return instance;
  }

  void init();
  void set_socket_fd(std::optional<int> fd);
  void start();
  void terminate();
  void add_client_fd(int fd);
  void enqueue(std::function<void()> task);
  void draw(const float* vp_mat);

  void consume_queued_tasks();

  uint32_t       next_serial();
  wl_event_loop* loop();

  std::unordered_map<wl_client*, wl_resource*>               output_map;
  std::unordered_map<wl_resource*, std::shared_ptr<Surface>> surface_map;
  std::unordered_map<wl_resource*,
      std::shared_ptr<proto::wayland::subcompositor::subsurface::Subsurface>>
      subsurface_map;

  std::unordered_map<wl_resource*,
      std::shared_ptr<core::data_device::DataDevice>>
      data_device_map;
  std::unordered_map<wl_resource*,
      std::shared_ptr<core::data_device::DataSource>>
      data_source_map;
  std::unordered_map<wl_resource*,
      std::shared_ptr<core::data_device::DataOffer>>
      data_offer_map;

  std::unordered_map<wl_resource*,
      std::shared_ptr<proto::xdg_shell::xdg_surface::XdgSurface>>
      xdg_surface_map;
  std::unordered_map<wl_resource*,
      std::shared_ptr<proto::xdg_shell::xdg_toplevel::XdgToplevel>>
      xdg_toplevel_map;
  std::unordered_map<wl_resource*,
      std::shared_ptr<proto::xdg_shell::xdg_popup::XdgPopup>>
      xdg_popup_map;

  server_seat::ServerSeat seat;
  RayCursor               ray_cursor;
  scene::Scene            scene;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  void draw_content(const float* vp_mat);

  bool is_initialized_ = false;

  render::RectRenderer               rect_renderer_;
  render::RayRenderer                ray_renderer_;
  wl_display*                        wl_display_             = nullptr;
  wl_event_source*                   task_source_            = nullptr;
  int                                task_dispatch_event_fd_ = -1;
  std::mutex                         task_mutex_;
  std::vector<std::function<void()>> tasks_;
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}

}  // namespace yabe::core::server
