#include "xdg-output-manager.hpp"

#include <xdg-output-protocol.h>

#include "xdg-output.hpp"

namespace yabe::proto::xdg::output {
namespace {
constexpr struct zxdg_output_manager_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_xdg_output =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* output_resource) {
          xdg_output::create(
              client, wl_resource_get_version(resource), id, output_resource);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zxdg_output_manager_v1_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::xdg::output
