#include "output.hpp"

#include <wayland-server.h>

#include "core/server.hpp"

namespace yabe::proto::wayland::output {
namespace {
constexpr struct wl_output_interface kImpl = {
    .release =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().output_map.erase(wl_resource_get_client(resource));
}
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_output_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  core::server::get().output_map.emplace(client, resource);
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  wl_output_send_geometry(resource, 0, 0, 0, 0, WL_OUTPUT_SUBPIXEL_UNKNOWN,
      "yabe-compositor", "output", WL_OUTPUT_TRANSFORM_NORMAL);
  // TODO: make resolution configurable by somehow
  // Note that the resolution depends on the device
  // so cannot decide on the compile time
  wl_output_send_mode(resource, WL_OUTPUT_MODE_CURRENT, 1920, 1080, 0);
  if (version >= WL_OUTPUT_SCALE_SINCE_VERSION) {
    wl_output_send_scale(resource, 1);
  }
  if (version >= WL_OUTPUT_NAME_SINCE_VERSION) {
    wl_output_send_name(resource, "WL-1");
  }
  if (version >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION) {
    wl_output_send_description(resource, "monitor");
  }
  if (version >= WL_OUTPUT_DONE_SINCE_VERSION) {
    wl_output_send_done(resource);
  }
}
}  // namespace yabe::proto::wayland::output
