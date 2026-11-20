#include "locked.hpp"

#include "core/scene/surface-node.hpp"
#include "core/server-seat/pointer-grab/default.hpp"
#include "core/server-seat/pointer-grab/implicit.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat::pointer_grab {
LockedPointerGrab LockedPointerGrab::instance;

LockedPointerGrab* LockedPointerGrab::start(
    const std::shared_ptr<scene::SurfaceNode>& locked_surface_node) {
  instance.locked_surface_node_ = locked_surface_node;
  locked_surface_node->surface()->locked_pointer().lock()->send_locked();
  return &instance;
}
bool LockedPointerGrab::can_start_with(
    const std::shared_ptr<scene::SurfaceNode>& node) {
  // at this time, grab is likely to Implicit; locking fires by click event
  if (!ImplicitPointerGrab::is_active() && !DefaultPointerGrab::is_active()) {
    return false;
  }
  if (server::get().seat.pointer.get_focused_scene_node().lock() != node) {
    return false;
  }
  const auto locked_pointer = node->surface()->locked_pointer().lock();
  return locked_pointer && locked_pointer->can_be_locked();
}

bool LockedPointerGrab::is_locking(const std::shared_ptr<
    proto::wayland::pointer_constraints::locked::LockedPointer>&
        locked_pointer) {
  if (!server::get().seat.pointer.is_active_grab(&instance)) {
    return false;
  }
  const auto node = instance.locked_surface_node_.lock();
  if (!node) {
    return false;
  }
  const auto surface_locked_pointer = node->surface()->locked_pointer();
  return surface_locked_pointer.lock() == locked_pointer;
}

void LockedPointerGrab::cleanup() {
  this->try_send_unlocked();
  this->locked_surface_node_.reset();
  server::get().seat.pointer.focus_pointed_surface();
}
void LockedPointerGrab::try_send_unlocked() {
  const auto node = this->locked_surface_node_.lock();
  if (!node) {
    return;
  }
  const auto locked_pointer = node->surface()->locked_pointer().lock();
  if (locked_pointer) {
    locked_pointer->send_unlocked();
  }
}

void LockedPointerGrab::move_relative(float /*dx*/, float /*dy*/) {
  // check whether lock target SurfaceNode is still valid here
  if (!this->locked_surface_node_.lock()) {
    server::get().seat.pointer.end_grab();
  }
}

void LockedPointerGrab::button(uint32_t button, bool is_pressed) {
  const auto node = this->locked_surface_node_.lock();
  if (!node) {
    server::get().seat.pointer.end_grab();
    return;
  }
  node->button(button, is_pressed);
  node->frame();
}
}  // namespace yabe::core::server_seat::pointer_grab
