#include "virtual-keyboard.hpp"

#include <virtual-keyboard-protocol.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <memory>

#include "core/keyboard.hpp"
#include "core/server.hpp"

namespace yabe::proto::wlr::virtual_keyboard {
namespace {
constexpr struct zwp_virtual_keyboard_v1_interface kImpl{
    .keymap =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t format,
            int32_t fd, uint32_t size) {
          const auto self =
              core::server::get().resources.get<core::Keyboard>(resource);
          if (self) {
            self->set_keymap(format, fd, size);
          }
        },
    .key =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t time,
            uint32_t key, uint32_t state) {
          const auto self =
              core::server::get().resources.get<core::Keyboard>(resource);
          if (!self) {
            return;
          }
          auto is_pressed = state == WL_KEYBOARD_KEY_STATE_PRESSED;
          self->handle_key(key, is_pressed);
          core::server::get().seat.keyboard.send_key(
              self, time, key, is_pressed);
        },
    .modifiers =
        [](wl_client* /*client*/, wl_resource* resource,
            uint32_t mods_depressed, uint32_t mods_latched,
            uint32_t mods_locked, uint32_t group) {
          const auto self =
              core::server::get().resources.get<core::Keyboard>(resource);
          if (!self) {
            return;
          }
          const core::ModMasks mods{
              .depressed = mods_depressed,
              .latched   = mods_latched,
              .locked    = mods_locked,
              .group     = group,
          };
          // check before handle_modifiers() as it may change self->mods
          const auto is_changed = self->mods() != mods;
          self->handle_modifiers(
              mods_depressed, mods_latched, mods_locked, group);
          if (is_changed) {
            core::server::get().seat.keyboard.send_modifiers(
                self, mods_depressed, mods_latched, mods_locked, group);
          }
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwp_virtual_keyboard_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  auto self = std::make_shared<core::Keyboard>();
  self->set_owner(client);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::wlr::virtual_keyboard
