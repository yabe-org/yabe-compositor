#include "locked-pointer.hpp"

#include <pointer-constraints-protocol.h>
#include <wayland-server-core.h>

#include <memory>

#include "core/server-seat/pointer-grab/locked.hpp"
#include "core/server.hpp"

namespace yabe::proto::wayland::pointer_constraints::locked {
LockedPointer::LockedPointer(
    wl_resource* resource, zwp_pointer_constraints_v1_lifetime lifetime)
    : resource_(resource), lifetime_(lifetime) {
}

void LockedPointer::send_locked() {
  this->has_been_locked_ = true;
  zwp_locked_pointer_v1_send_locked(this->resource_);
}

void LockedPointer::send_unlocked() {
  if (this->resource_) {
    zwp_locked_pointer_v1_send_unlocked(this->resource_);
  }
}
void LockedPointer::invalidate_resource() {
  this->resource_ = nullptr;
}
bool LockedPointer::can_be_locked() {
  if (this->lifetime_ == ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT) {
    return true;
  }
  return !this->has_been_locked_;
}

namespace {
constexpr struct zwp_locked_pointer_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_cursor_position_hint =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_fixed_t /*surface_x*/, wl_fixed_t /*surface_y*/) {
          // TODO
        },
    .set_region =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*region*/) {
          // TODO
        },

};

void handle_destruction(wl_resource* resource) {
  auto self = core::server::get().resources.get<LockedPointer>(resource);
  if (!self) {
    return;
  }
  self->invalidate_resource();
  if (core::server_seat::pointer_grab::LockedPointerGrab::is_locking(self)) {
    core::server::get().seat.pointer.end_grab();
  }
  core::server::get().resources.remove(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<core::Surface>& surface,
    zwp_pointer_constraints_v1_lifetime   lifetime) {
  wl_resource* resource =
      wl_resource_create(client, &zwp_locked_pointer_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<LockedPointer>(resource, lifetime);
  core::server::get().resources.add(resource, self);
  if (!surface->set_locked_pointer(self)) {
    wl_resource_post_error(resource,
        ZWP_POINTER_CONSTRAINTS_V1_ERROR_ALREADY_CONSTRAINED,
        "surface already has locked_pointer");
    return;
  }

  const auto node = surface->surface_node();
  using namespace core::server_seat::pointer_grab;  // NOLINT
  if (node && LockedPointerGrab::can_start_with(*node)) {
    core::server::get().seat.pointer.start_grab(
        LockedPointerGrab::start(*node));
  }
}
}  // namespace yabe::proto::wayland::pointer_constraints::locked
