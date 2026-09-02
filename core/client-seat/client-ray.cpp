#include "client-ray.hpp"

#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/server.hpp"

namespace yabe::core::client_seat {
namespace {
wl_array make_wl_array_from_vec3(const glm::vec3& vec) {
  return wl_array{
      .size  = sizeof(glm::vec3),
      .alloc = 0,
      .data = (void*)glm::value_ptr(vec),  // NOLINT(google-readability-casting)
  };
}
}  // namespace

void ClientRay::add_ray(wl_resource* resource) {
  LOG_DBG("add ray#{}", wl_resource_get_id(resource));
  this->rays_.push_back(resource);
}
void ClientRay::remove_ray(wl_resource* resource) {
  this->rays_.remove(resource);
}

void ClientRay::send_enter(
    const std::shared_ptr<proto::zwin::VirtualObject>& target,
    const glm::vec3& origin, const glm::vec3& direction) {
  const auto serial          = server::get().next_serial();
  auto       origin_array    = make_wl_array_from_vec3(origin);
  auto       direction_array = make_wl_array_from_vec3(direction);
  for (auto* resource : this->rays_) {
    zwn_ray_send_enter(
        resource, serial, target->resource(), &origin_array, &direction_array);
  }
}

void ClientRay::send_leave(
    const std::shared_ptr<proto::zwin::VirtualObject>& target) {
  const auto serial = server::get().next_serial();
  for (auto* resource : this->rays_) {
    zwn_ray_send_leave(resource, serial, target->resource());
  }
}

void ClientRay::send_motion(
    const glm::vec3& origin, const glm::vec3& direction) {
  const auto serial          = server::get().next_serial();
  auto       origin_array    = make_wl_array_from_vec3(origin);
  auto       direction_array = make_wl_array_from_vec3(direction);
  for (auto* resource : this->rays_) {
    zwn_ray_send_motion(resource, serial, &origin_array, &direction_array);
  }
}

void ClientRay::send_button(uint32_t button, zwn_ray_button_state state) {
  const auto serial = server::get().next_serial();
  const auto now    = common::now_msec();
  for (auto* resource : this->rays_) {
    zwn_ray_send_button(resource, serial, now, button, state);
  }
}

void ClientRay::send_axis(zwn_ray_axis axis, float value) {
  const auto serial      = server::get().next_serial();
  const auto fixed_value = wl_fixed_from_double(value);
  for (auto* resource : this->rays_) {
    zwn_ray_send_axis(resource, serial, axis, fixed_value);
  }
}

void ClientRay::send_frame() {
  for (auto* resource : this->rays_) {
    zwn_ray_send_frame(resource);
  }
}
}  // namespace yabe::core::client_seat
