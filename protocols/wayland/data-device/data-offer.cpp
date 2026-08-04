#include "data-offer.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol-core.h>

namespace yabe::proto::wayland::data_device_manager::data_offer {
namespace {
void accept(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*serial*/, const char* /*mime_type*/) {
  // TODO
}
void receive(wl_client* /*client*/, wl_resource* /*resource*/,
    const char* /*mime_type*/, int32_t /*fd*/) {
  // TODO
}
void destroy(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void finish(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_actions(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*dnd_actions*/, uint32_t /*preferred_action*/) {
  // TODO
}
constexpr struct wl_data_offer_interface kImpl = {
    .accept      = accept,
    .receive     = receive,
    .destroy     = destroy,
    .finish      = finish,
    .set_actions = set_actions,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_offer_interface, wl_data_offer_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yabe::proto::wayland::data_device_manager::data_offer
