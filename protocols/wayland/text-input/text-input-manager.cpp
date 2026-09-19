#include "text-input-manager.hpp"

#include <text-input-protocol.h>
#include <wayland-server-core.h>

#include "text-input.hpp"

namespace yabe::proto::wayland::text_input {
namespace {
constexpr struct zwp_text_input_manager_v3_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_text_input =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* /*seat*/) {
          create_text_input(client, wl_resource_get_version(resource), id);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(client,
      &zwp_text_input_manager_v3_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}

}  // namespace yabe::proto::wayland::text_input
