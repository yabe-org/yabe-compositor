#include "virtual-keyboard-manager.hpp"

#include <virtual-keyboard-protocol.h>
#include <wayland-server-core.h>

#include <wlr/virtual-keyboard/virtual-keyboard.hpp>

namespace yabe::proto::wlr::virtual_keyboard {
namespace {
constexpr struct zwp_virtual_keyboard_manager_v1_interface kImpl{
    .create_virtual_keyboard =
        [](wl_client* client, wl_resource* resource, wl_resource* /*seat*/,
            uint32_t id) {
          create(client, wl_resource_get_version(resource), id);
        },
};
}  // namespace
void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwp_virtual_keyboard_manager_v1_interface,
          static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wlr::virtual_keyboard
