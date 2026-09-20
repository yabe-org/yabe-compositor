#include "input-method-manager.hpp"

#include <input-method-protocol.h>
#include <wayland-server-core.h>

#include <wlr/input-method/input-method.hpp>

namespace yabe::proto::wlr::input_method {
namespace {
constexpr struct zwp_input_method_manager_v2_interface kImpl = {
    .get_input_method =
        [](struct wl_client* client, struct wl_resource* resource,
            struct wl_resource* /*seat*/, uint32_t       id) {
          create_input_method(client, wl_resource_get_version(resource), id);
        },
    .destroy =
        [](struct wl_client* /*client*/, struct wl_resource* resource) {
          wl_resource_destroy(resource);
        },

};  // namespace
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(client,
      &zwp_input_method_manager_v2_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wlr::input_method
