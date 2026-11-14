#include "importer.hpp"

#include <wayland-server-core.h>
#include <xdg-foreign-protocol.h>

#include "core/server.hpp"
#include "imported.hpp"

namespace yabe::proto::xdg::foreign::importer {
namespace {
constexpr struct zxdg_importer_v2_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .import_toplevel =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            const char* handle) {
          // whether exported is valid or invalid (not found, already dead),
          // Imported should be created and be sent destroyed()
          std::weak_ptr<exported::Exported> exported;
          const auto it = core::server::get().exported_map.find(handle);
          if (it != core::server::get().exported_map.end()) {
            exported = it->second;
          }
          imported::create(
              client, wl_resource_get_version(resource), id, exported);
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zxdg_importer_v2_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::xdg::foreign::importer
