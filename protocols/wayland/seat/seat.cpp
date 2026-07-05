#include "seat.hpp"

#include <wayland-server.h>

#include "core/server.hpp"
#include "pointer.hpp"

namespace yabe::proto::wayland::seat {
namespace {
void get_pointer(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  pointer::create(client, id);
}
void get_keyboard(
    wl_client*  /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void get_touch(wl_client* client, wl_resource* /*resource*/, uint32_t /*id*/) {
  wl_client_post_implementation_error(
      client, "touch device is not implemented");
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_seat_interface kImpl = {
    .get_pointer  = get_pointer,
    .get_keyboard = get_keyboard,
    .get_touch    = get_touch,
    .release      = release,
};

void destroy(wl_resource* resource) {
  auto* client = wl_resource_get_client(resource);
  core::server::get().seat.remove_client_seat(client);
}
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* client_seat = core::server::get().seat.create_client_seat(client);
  wl_resource_set_implementation(resource, &kImpl, client_seat, destroy);
  wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER);
}
}  // namespace yabe::proto::wayland::seat
