#include "keyboard-grab.hpp"

#include <input-method-protocol.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "core/server.hpp"

namespace yabe::proto::wlr::input_method {
KeyboardGrab::KeyboardGrab(wl_resource* resource) : resource_(resource) {
}

void KeyboardGrab::send_keymap(uint32_t format, int32_t fd, uint32_t size) {
  zwp_input_method_keyboard_grab_v2_send_keymap(
      this->resource_, format, fd, size);
}
void KeyboardGrab::send_repeat_info(int32_t rate, int32_t delay) {
  zwp_input_method_keyboard_grab_v2_send_repeat_info(
      this->resource_, rate, delay);
}
void KeyboardGrab::send_key(
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
  zwp_input_method_keyboard_grab_v2_send_key(
      this->resource_, serial, time, key, state);
}
void KeyboardGrab::send_modifiers(uint32_t serial, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
  zwp_input_method_keyboard_grab_v2_send_modifiers(this->resource_, serial,
      mods_depressed, mods_latched, mods_locked, group);
}

namespace {
constexpr struct zwp_input_method_keyboard_grab_v2_interface kImpl{
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  auto self = core::server::get().resources.get<KeyboardGrab>(resource);
  if (self) {
    core::server::get().seat.keyboard.try_remove_keyboard_grab(self);
    core::server::get().resources.remove(resource);
  }
}
}  // namespace

void create_keyboard_grab(wl_client* client, int version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwp_input_method_keyboard_grab_v2_interface, version, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  auto self = std::make_shared<KeyboardGrab>(resource);
  core::server::get().resources.add(resource, self);
  core::server::get().seat.keyboard.set_keyboard_grab(self);
}
}  // namespace yabe::proto::wlr::input_method
