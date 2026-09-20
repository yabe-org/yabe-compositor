#include "input-method.hpp"

#include <input-method-protocol.h>
#include <wayland-server-core.h>
#include <wayland-server.h>

#include "core/server.hpp"

namespace yabe::proto::wlr::input_method {
namespace {
constexpr struct zwp_input_method_v2_interface kImpl{
    .commit_string =
        [](wl_client* /*client*/, wl_resource* resource, const char* text) {
          core::server::get().seat.input_method.set_commit_string(
              resource, text);
        },
    .set_preedit_string =
        [](wl_client* /*client*/, wl_resource* resource, const char* text,
            int32_t cursor_begin, int32_t cursor_end) {
          core::server::get().seat.input_method.set_preedit_string(
              resource, text, cursor_begin, cursor_end);
        },
    .delete_surrounding_text =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t before_length,
            uint32_t after_length) {
          // TODO
          core::server::get().seat.input_method.delete_surrounding_text(
              resource, before_length, after_length);
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t serial) {
          core::server::get().seat.input_method.commit(resource, serial);
        },
    .get_input_popup_surface =
        [](wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/,
            wl_resource* /*surface*/) {
          // TODO
        },
    .grab_keyboard =
        [](wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
          // TODO
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().seat.input_method.try_clear_current_input_method(
      resource);
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
  core::server::get().seat.input_method.set_input_method(resource);
}
}  // namespace yabe::proto::wlr::input_method
