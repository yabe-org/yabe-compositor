#include "input-method.hpp"

#include <input-method-protocol.h>
#include <wayland-server-core.h>
#include <wayland-server.h>

#include <memory>
#include <wlr/input-method/input-popup.hpp>

#include "core/server.hpp"
#include "core/surface.hpp"
#include "protocols/wlr/input-method/keyboard-grab.hpp"

namespace yabe::proto::wlr::input_method {
InputMethod::InputMethod(wl_resource* resource) : resource_(resource) {
}
uint32_t InputMethod::serial() {
  return this->serial_++;
}

namespace {
constexpr struct zwp_input_method_v2_interface kImpl{
    .commit_string =
        [](wl_client* /*client*/, wl_resource* resource, const char* text) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (self) {
            core::server::get().seat.input_method.set_commit_string(self, text);
          }
        },
    .set_preedit_string =
        [](wl_client* /*client*/, wl_resource* resource, const char* text,
            int32_t cursor_begin, int32_t cursor_end) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (self) {
            core::server::get().seat.input_method.set_preedit_string(
                self, text, cursor_begin, cursor_end);
          }
        },
    .delete_surrounding_text =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t before_length,
            uint32_t after_length) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (self) {
            core::server::get().seat.input_method.delete_surrounding_text(
                self, before_length, after_length);
          }
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t /*serial*/) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (self) {
            core::server::get().seat.input_method.commit(self);
          }
        },
    .get_input_popup_surface =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface_resource) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (!self) {
            return;
          }
          if (core::server::get().seat.input_method.has_input_popup()) {
            return;
          }
          auto surface = core::server::get().resources.get<core::Surface>(
              surface_resource);
          if (surface) {
            create_input_popup(
                client, wl_resource_get_version(resource), id, self, surface);
          }
        },
    .grab_keyboard =
        [](wl_client* client, wl_resource* resource, uint32_t id) {
          auto self = core::server::get().resources.get<InputMethod>(resource);
          if (core::server::get().seat.input_method.is_current_input_method(
                  self)) {
            create_keyboard_grab(client, wl_resource_get_version(resource), id);
          }
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  auto self = core::server::get().resources.get<InputMethod>(resource);
  if (!self) {
    return;
  }
  core::server::get().seat.input_method.try_clear_current_input_method(self);
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_input_method(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwp_input_method_v2_interface, version, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  auto self = std::make_shared<InputMethod>(resource);
  core::server::get().seat.input_method.set_input_method(self);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::wlr::input_method
