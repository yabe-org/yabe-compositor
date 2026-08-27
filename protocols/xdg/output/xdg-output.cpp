#include "xdg-output.hpp"

#include <xdg-output-protocol.h>

#include "protocols/wayland/output.hpp"
#include "wayland-server-protocol.h"

namespace yabe::proto::xdg::output::xdg_output {
namespace {
constexpr struct zxdg_output_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
}  // namespace

void create(
    wl_client* client, int version, uint32_t id, wl_resource* output_resource) {
  wl_resource* resource =
      wl_resource_create(client, &zxdg_output_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);

  zxdg_output_v1_send_logical_position(resource, 0, 0);
  zxdg_output_v1_send_logical_size(resource,
      wayland::output::get_viewport_width(),
      wayland::output::get_viewport_height());
  if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION) {
    zxdg_output_v1_send_name(resource, "WL-1");
  }
  if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION) {
    zxdg_output_v1_send_description(resource, "monitor");
  }
  // xdg_output.done is deprecated in v3
  // and compositor must send wl_output.done instead
  // (note that wl_output.done can be sent multiple times)
  if (version < 3) {
    zxdg_output_v1_send_done(resource);
  } else {
    wl_output_send_done(output_resource);
  }
}
}  // namespace yabe::proto::xdg::output::xdg_output
