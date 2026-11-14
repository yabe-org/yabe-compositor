#include "exporter.hpp"

#include <wayland-server-core.h>
#include <xdg-foreign-protocol.h>
#include <xdg-shell-protocol.h>

#include "core/server.hpp"
#include "core/surface.hpp"
#include "exported.hpp"
#include "protocols/xdg/shell/xdg-toplevel.hpp"

namespace yabe::proto::xdg::foreign::exporter {
namespace {
constexpr struct zxdg_exporter_v2_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .export_toplevel =
        [](wl_client* client, wl_resource* resource, uint32_t id,
            wl_resource* surface_resource) {
          auto surface = core::server::get().resources.get<core::Surface>(
              surface_resource);
          if (!surface) {
            wl_client_post_implementation_error(
                client, "invalid surface resource");
            return;
          }
          if (surface->is_role_of<shell::xdg_toplevel::XdgToplevel>()) {
            exported::create(
                client, wl_resource_get_version(resource), id, surface);
          } else {
            wl_resource_post_error(resource,
                ZXDG_EXPORTER_V2_ERROR_INVALID_SURFACE,
                "surface role is not xdg_toplevel");
          }
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zxdg_exporter_v2_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::xdg::foreign::exporter
