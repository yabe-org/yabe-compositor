#pragma once

#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include <glm/glm.hpp>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "protocols/zwin/virtual-object.hpp"

namespace yabe::core::client_seat {
class ClientRay {
 public:
  DISABLE_MOVE_AND_COPY(ClientRay);
  explicit ClientRay() = default;
  ~ClientRay()         = default;

  void add_ray(wl_resource* resource);
  void remove_ray(wl_resource* resource);

  void send_enter(const std::shared_ptr<proto::zwin::VirtualObject>& target,
      const glm::vec3& origin, const glm::vec3& direction);
  void send_leave(const std::shared_ptr<proto::zwin::VirtualObject>& target);
  void send_motion(const glm::vec3& origin, const glm::vec3& direction);
  void send_button(uint32_t button, zwn_ray_button_state state);
  void send_axis(zwn_ray_axis axis, float value);
  void send_frame();

 private:
  std::list<wl_resource*> rays_;
};
}  // namespace yabe::core::client_seat
