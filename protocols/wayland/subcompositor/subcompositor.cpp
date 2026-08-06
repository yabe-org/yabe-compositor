#include "subcompositor.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "core/server.hpp"
#include "subsurface.hpp"

namespace yabe::proto::wayland::subcompositor {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_subsurface(struct wl_client* client, struct wl_resource* resource,
    uint32_t id, struct wl_resource* surface, struct wl_resource* parent) {
  auto parent_surface = core::server::get().surface_map.find(parent);
  if (parent_surface == core::server::get().surface_map.end()) {
    wl_client_post_implementation_error(wl_resource_get_client(resource),
        "subsurface requested for unknown parent surface");
    return;
  }
  auto wl_surface = core::server::get().surface_map.at(surface);
  subsurface::create(client, id, wl_surface, parent_surface->second);
}
constexpr struct wl_subcompositor_interface kImpl = {
    .destroy        = destroy,
    .get_subsurface = get_subsurface,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_subcompositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::subcompositor
