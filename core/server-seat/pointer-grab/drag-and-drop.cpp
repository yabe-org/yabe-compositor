#include "drag-and-drop.hpp"

#include <linux/input-event-codes.h>
#include <sys/types.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

#include "core/data-device/data-offer.hpp"
#include "core/data-device/data-source.hpp"
#include "core/server.hpp"
#include "core/surface.hpp"
#include "protocols/wayland/data-device/data-offer.hpp"

namespace yabe::core::server_seat::pointer_grab {
DragAndDropPointerGrab* DragAndDropPointerGrab::start(
    std::weak_ptr<data_device::DataSource>&& data_source) {
  static DragAndDropPointerGrab instance;
  // if drag-and-drop is already in progress and the data_source is changed,
  // send wl_data_source.canceled for the previous one
  if (server::get().seat.pointer.is_active_grab(&instance) &&
      instance.data_source_.lock() != data_source.lock()) {
    instance.data_source_.lock()->send_cancelled();
  }
  server::get().seat.pointer.unfocus_surface();
  instance.selected_action_    = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  instance.prev_action_        = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  instance.data_source_        = std::move(data_source);
  instance.current_data_offer_ = std::weak_ptr<data_device::DataOffer>();
  instance.entered_surface_    = std::weak_ptr<Surface>();
  instance.enter_pointed_surface();
  return &instance;
}

void DragAndDropPointerGrab::cleanup() {
  const auto entered_surface = this->entered_surface_.lock();
  const auto client_seat = entered_surface ? server::get().seat.get_client_seat(
                                                 entered_surface->client()) :
                                             std::nullopt;
  if (client_seat) {
    client_seat.value()->data_device.send_drop();
  }
  if (this->data_source_.lock()) {
    this->data_source_.lock()->send_dnd_drop_performed();
  }
  this->send_cancel_if_needed();
  if (client_seat) {
    client_seat.value()->data_device.send_leave();
  }

  server::get().ray_cursor.set_drag_and_drop_icon({});
  server::get().seat.pointer.focus_pointed_surface();
}
void DragAndDropPointerGrab::send_cancel_if_needed() {
  const auto source = this->data_source_.lock();
  if (!source) {
    return;
  }
  // for objects of version <= 2,
  // send cancel only if the data source is replaced
  if (wl_resource_get_version(source->resource()) <= 2) {
    return;
  }
  // for objects of version > 2, send cancel if:
  // drop destination did not accept any of the mime,
  const auto offer = this->current_data_offer_.lock();
  if (offer && !offer->did_accept_mime()) {
    return;
  }
  // drop destination did not select any action,
  if (this->selected_action_ != WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE) {
    return;
  }
  // or drop operation did not happen over a surface
  if (this->entered_surface_.lock()) {
    return;
  }
  source->send_cancelled();
}

void DragAndDropPointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(dy, dx);
  this->enter_pointed_surface();
}

void DragAndDropPointerGrab::button(uint32_t button, bool is_pressed) {
  // ensure that the latest action is sent
  this->renew_action();
  if (button == BTN_LEFT && !is_pressed) {
    server::get().seat.pointer.end_grab();
  }
}

void DragAndDropPointerGrab::enter_pointed_surface() {
  const auto intersection =
      server::get().workspace.current_scene().get_nearest_intersection(
          server::get().ray_cursor.origin(),
          server::get().ray_cursor.direction());

  if (!intersection.has_value()) {
    this->set_entered_surface({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  const auto surface_node =
      std::dynamic_pointer_cast<scene::SurfaceNode>(intersection.value().node);
  if (!surface_node) {
    this->set_entered_surface({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  const auto surface = surface_node->surface();
  if (!surface) {
    this->set_entered_surface({});
    server::get().ray_cursor.reset_cursor();
    return;
  }
  server::get().ray_cursor.set_length(
      intersection.value().intersect_info.distance);

  auto*       client      = surface_node->surface()->client();
  const auto& client_seat = server::get().seat.get_client_seat(client);
  if (!client_seat) {
    server::get().seat.pointer.end_grab();
    return;
  }
  const auto is_focus_changed = this->set_entered_surface(surface);
  if (is_focus_changed) {
    const auto data_source = this->data_source_.lock();
    std::optional<std::shared_ptr<data_device::DataOffer>> offer = std::nullopt;
    // create and send data_offer if source is available
    if (data_source) {
      offer = proto::wayland::data_device_manager::data_offer::create(
          client, data_source);
      this->current_data_offer_ = *offer;
      client_seat.value()->data_device.send_data_offer(*offer);
      data_source->for_each_mime([&offer](const std::string& mime) {
        offer.value()->send_offer(mime);
      });
      offer.value()->send_source_actions(data_source->dnd_actions());
    }
    // send enter event regardless of data_offer creation
    client_seat.value()->data_device.send_enter(surface_node->surface(),
        intersection.value().intersect_info.pos.x,
        intersection.value().intersect_info.pos.y, offer);
  }
  client_seat.value()->data_device.send_motion(
      intersection.value().intersect_info.pos.x,
      intersection.value().intersect_info.pos.y);
  this->renew_action();
}
bool DragAndDropPointerGrab::set_entered_surface(std::weak_ptr<Surface>
        surface) {  // NOLINT(performance-unnecessary-value-param)
  const auto locked_surface          = surface.lock();
  const auto current_focused_surface = this->entered_surface_.lock();
  if (current_focused_surface == locked_surface) {
    return false;
  }
  if (current_focused_surface) {
    const auto& client_seat =
        server::get().seat.get_client_seat(current_focused_surface->client());
    if (client_seat) {
      client_seat.value()->data_device.send_leave();
    }
  }

  this->entered_surface_ = surface;
  return true;
}

void DragAndDropPointerGrab::renew_action() {
  this->selected_action_ = this->select_action();
  if (this->selected_action_ == this->prev_action_) {
    return;
  }
  this->prev_action_ = this->selected_action_;

  if (this->data_source_.lock()) {
    this->data_source_.lock()->send_action(this->selected_action_);
  }
  if (this->current_data_offer_.lock()) {
    this->current_data_offer_.lock()->send_action(this->selected_action_);
  }
}

namespace {
wl_data_device_manager_dnd_action get_action_selected_by_keyboard() {
  const auto keyboard = server::get().seat.keyboard.current_keyboard();
  if (keyboard->is_mod_pressed(ModKeys::SHIFT)) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
  }
  if (keyboard->is_mod_pressed(ModKeys::CTRL)) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
  }
  if (keyboard->is_key_pressed(XKB_KEY_Q)) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
  }
  return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
}
}  // namespace
wl_data_device_manager_dnd_action DragAndDropPointerGrab::select_action() {
  const auto source          = this->data_source_.lock();
  const auto current_offer   = this->current_data_offer_.lock();
  const auto entered_surface = this->entered_surface_.lock();
  if (!source || !current_offer || !entered_surface) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
  }
  const auto available_actions =
      source->dnd_actions() & current_offer->dnd_actions();
  // prioritize an action selected based on key press
  const auto keyboard_selected_action = get_action_selected_by_keyboard();
  if (available_actions & keyboard_selected_action) {
    return keyboard_selected_action;
  }
  // next, use preferred_action if it is requested by data_offer
  if (available_actions & current_offer->preferred_action()) {
    return current_offer->preferred_action();
  }
  // finally, use one of supported actions in the order of copy, move, ask
  if (available_actions & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
  }
  if (available_actions & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
  }
  if (available_actions & WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) {
    return WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;
  }
  return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
}
}  // namespace yabe::core::server_seat::pointer_grab
