#include "output.hpp"

#include <wayland-server.h>

#include "common/thread-safety.hpp"
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
  core::server::get().output_map.erase(resource->client);
}

common::Mutex              mutex;
ViewportSize viewport_size GUARDED_BY(mutex){.width = 1920, .height = 1080};
}  // namespace
void set_viewport_size(int32_t width, int32_t height) {
  const auto lock = mutex.lock();
  viewport_size   = {.width = width, .height = height};
}
ViewportSize get_viewport_size() {
  const auto lock = mutex.lock();
  return viewport_size;
}

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_output_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  core::server::get().output_map.emplace(client, resource);
  wl_output_send_geometry(resource, 0, 0, 0, 0, WL_OUTPUT_SUBPIXEL_UNKNOWN,
      "yabe-compositor", "output", WL_OUTPUT_TRANSFORM_NORMAL);
  const auto viewport = get_viewport_size();
  wl_output_send_mode(
      resource, WL_OUTPUT_MODE_CURRENT, viewport.width, viewport.height, 0);
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
