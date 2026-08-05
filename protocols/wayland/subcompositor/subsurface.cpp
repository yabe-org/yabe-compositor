#include "subsurface.hpp"

#include <wayland-server-protocol-core.h>

namespace yabe::proto::wayland::subcompositor::subsurface {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_position(struct wl_client* /*client*/,
    struct wl_resource* /*resource*/, int32_t /*x*/, int32_t /*y*/) {
  // TODO
}
void place_above(struct wl_client* /*client*/, struct wl_resource* /*resource*/,
    struct wl_resource* /*sibling*/) {
  // TODO
}
void place_below(struct wl_client* /*client*/, struct wl_resource* /*resource*/,
    struct wl_resource* /*sibling*/) {
  // TODO
}
void set_sync(struct wl_client* /*client*/, struct wl_resource* /*resource*/) {
  // TODO
}
void set_desync(
    struct wl_client* /*client*/, struct wl_resource* /*resource*/) {
  // TODO
}
constexpr struct wl_subsurface_interface kImpl = {
    .destroy      = destroy,
    .set_position = set_position,
    .place_above  = place_above,
    .place_below  = place_below,
    .set_sync     = set_sync,
    .set_desync   = set_desync,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_subsurface_interface, wl_subsurface_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::subcompositor::subsurface
