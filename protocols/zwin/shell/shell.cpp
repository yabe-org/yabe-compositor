#include "shell.hpp"

#include <zwin-shell-protocol.h>

#include <glm/glm.hpp>

#include "bounded.hpp"
#include "core/server.hpp"
#include "expansive.hpp"

namespace yabe::proto::zwin::shell {
namespace {
constexpr struct zwn_shell_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .get_bounded =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* virtual_object_resource, wl_array* half_size) {
          auto virtual_object =
              core::server::get().resources.get<VirtualObject>(
                  virtual_object_resource);
          if (!virtual_object) {
            wl_client_post_implementation_error(client,
                "zwn_shell.get_bounded: invalid virtual_object resource");
            return;
          }
          create_bounded(client, wl_resource_get_version(resource), id,
              virtual_object, half_size);
        },
    .get_expansive =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* virtual_object_resource) {
          auto virtual_object =
              core::server::get().resources.get<VirtualObject>(
                  virtual_object_resource);
          if (!virtual_object) {
            wl_client_post_implementation_error(client,
                "zwn_shell.get_expansive: invalid virtual_object resource");
            return;
          }
          create_expansive(
              client, wl_resource_get_version(resource), id, virtual_object);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_shell_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::zwin::shell
