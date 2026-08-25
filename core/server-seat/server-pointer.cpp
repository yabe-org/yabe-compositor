#include "server-pointer.hpp"

#include <wayland-server.h>

#include <optional>

#include "core/scene/surface-node.hpp"
#include "core/server-seat/pointer-grab/default.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
ServerPointer::ServerPointer()
    : grab_(pointer_grab::DefaultPointerGrab::start()) {
}

void ServerPointer::unfocus_surface() {
  this->set_focused_surface_node({});
}
void ServerPointer::focus_pointed_surface() {
  const auto intersection = server::get().scene.get_nearest_intersection(
      server::get().ray_cursor.origin(), server::get().ray_cursor.direction());

  if (!intersection.has_value()) {
    this->set_focused_surface_node({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  const auto surface_node =
      std::dynamic_pointer_cast<scene::SurfaceNode>(intersection.value().node);
  if (!surface_node) {
    this->set_focused_surface_node({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  server::get().ray_cursor.set_length(
      intersection.value().intersect_info.distance);
  server::get().ray_cursor.set_cursor_rotation(
      intersection.value().node->geometry().rotation());

  const auto& client_seat =
      server::get().seat.get_client_seat(surface_node->surface()->client());
  if (!client_seat) {
    this->set_focused_surface_node({});
    return;
  }

  const auto is_focus_changed = this->set_focused_surface_node(surface_node);
  if (is_focus_changed) {
    client_seat.value()->pointer.send_enter(surface_node->surface()->resource(),
        intersection.value().intersect_info.pos.x,
        intersection.value().intersect_info.pos.y);
  }
  client_seat.value()->pointer.send_motion(
      intersection.value().intersect_info.pos.x,
      intersection.value().intersect_info.pos.y);
  client_seat.value()->pointer.send_frame();
}

void ServerPointer::start_grab(pointer_grab::PointerGrab* grab) {
  assert(grab);
  this->grab_ = grab;
}
void ServerPointer::end_grab() {
  this->grab_->cleanup();
  this->grab_ = pointer_grab::DefaultPointerGrab::start();
}

void ServerPointer::handle_pointer_move_relative(float dx, float dy) {
  this->grab_->move_relative(dx, dy);
}
void ServerPointer::handle_pointer_button(uint32_t button, bool is_pressed) {
  this->grab_->button(button, is_pressed);
}
// NOLINTBEGIN(readability-make-member-function-const): to keep consistency with
// other handle_pointer_* functions
void ServerPointer::handle_pointer_axis(uint32_t axis, float value) {
  const auto& client_seat = this->get_focused_client_seat();
  if (client_seat) {
    client_seat.value()->pointer.send_axis(axis, value);
    client_seat.value()->pointer.send_frame();
  }
}
// NOLINTEND(readability-make-member-function-const)

std::optional<std::shared_ptr<client_seat::ClientSeat>>
ServerPointer::get_focused_client_seat() const {
  auto surface_node = this->focused_surface_node_.lock();
  if (!surface_node) {
    return std::nullopt;
  }
  return server::get().seat.get_client_seat(surface_node->surface()->client());
}

bool ServerPointer::set_focused_surface_node(std::weak_ptr<scene::SurfaceNode>
        surface_node) {  // NOLINT(performance-unnecessary-value-param)
  const auto current_focused_surface_node = this->focused_surface_node_.lock();
  if (current_focused_surface_node == surface_node.lock()) {
    return false;
  }
  if (current_focused_surface_node) {
    const auto& surface = current_focused_surface_node->surface();
    const auto& client_seat =
        server::get().seat.get_client_seat(surface->client());
    if (client_seat) {
      client_seat.value()->pointer.send_leave(surface->resource());
      client_seat.value()->pointer.send_frame();
    }
  }

  this->focused_surface_node_ = surface_node;
  return true;
}
}  // namespace yabe::core::server_seat
