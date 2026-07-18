#include "server-pointer.hpp"

#include <optional>

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/default.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
ServerPointer::ServerPointer()
    : grab_(pointer_grab::DefaultPointerGrab::start()) {
}

void ServerPointer::unfocus_surface() {
  this->set_focused_surface(std::weak_ptr<Surface>());
}
void ServerPointer::focus_pointed_surface() {
  const auto intersection = server::get().surfaces.get_nearest_intersection(
      server::get().ray_cursor.origin(), server::get().ray_cursor.direction());

  if (!intersection.has_value()) {
    this->set_focused_surface(std::weak_ptr<Surface>());
    return;
  }
  const auto& [surface, intersect_info] = intersection.value();
  const auto locked_surface             = surface.lock();
  if (!locked_surface) {
    this->set_focused_surface(std::weak_ptr<Surface>());
    return;
  }

  const auto cursor_surface = this->get_cursor_surface().lock();
  if (cursor_surface) {
    const auto cursor_distance = intersect_info.distance * 0.98F;
    cursor_surface->move_absolute(
        server::get().ray_cursor.origin() +
        server::get().ray_cursor.direction() * cursor_distance);
  }

  const auto& client_seat =
      server::get().seat.get_client_seat(locked_surface->client());
  if (!client_seat) {
    this->set_focused_surface(std::weak_ptr<Surface>());
    return;
  }

  const auto is_focus_changed = this->set_focused_surface(surface);
  if (is_focus_changed) {
    client_seat.value()->pointer.send_enter(
        locked_surface->resource(), intersect_info.pos.x, intersect_info.pos.y);
  }
  client_seat.value()->pointer.send_motion(
      intersect_info.pos.x, intersect_info.pos.y);
  client_seat.value()->pointer.send_frame();
}

void ServerPointer::set_surface_as_cursor(
    wl_resource* surface, int32_t hotspot_x, int32_t hotspot_y) {
  if (surface == nullptr) {
    this->cursor_surface_ = std::weak_ptr<Surface>();
    return;
  }
  auto cursor_surface = server::get().surfaces.get(surface).lock();
  if (!cursor_surface) {
    LOG_ERR("cursor surface not found");
    return;
  }
  const auto cursor_distance = server::get().ray_cursor.length() * 0.98F;
  cursor_surface->set_type(SurfaceType::CURSOR);
  cursor_surface->move_absolute(
      server::get().ray_cursor.origin() +
      server::get().ray_cursor.direction() * cursor_distance);
  this->cursor_surface_ = cursor_surface;
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
void ServerPointer::handle_pointer_axis(uint32_t axis, float value) {
  const auto& client_seat = this->get_focused_client_seat();
  if (client_seat) {
    client_seat.value()->pointer.send_axis(axis, value);
    client_seat.value()->pointer.send_frame();
  }
}

std::optional<std::shared_ptr<client_seat::ClientSeat>>
ServerPointer::get_focused_client_seat() const {
  auto focused_surface = this->focused_surface_.lock();
  if (!focused_surface) {
    return std::nullopt;
  }
  return server::get().seat.get_client_seat(focused_surface->client());
}

bool ServerPointer::set_focused_surface(std::weak_ptr<Surface> surface) {
  if (this->focused_surface_.lock() == surface.lock()) {
    return false;
  }
  if (auto focused_surface = this->focused_surface_.lock()) {
    const auto& client_seat =
        server::get().seat.get_client_seat(focused_surface->client());
    if (client_seat) {
      client_seat.value()->pointer.send_leave(focused_surface->resource());
      client_seat.value()->pointer.send_frame();
    }
  }
  this->focused_surface_ = surface;
  return true;
}
}  // namespace yabe::core::server_seat
