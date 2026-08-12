#include "surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <optional>

#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wayland::surface {
namespace {
constexpr struct wl_surface_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .attach =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* buffer_resource, int32_t /*dx*/, int32_t /*dy*/) {
          core::server::get().surface_map.at(resource)->attach(buffer_resource);
        },
    .damage =
        [](wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
            int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
          // TODO
        },
    .frame =
        [](wl_client* client, wl_resource* resource, uint32_t callback) {
          wl_resource* callback_resource = wl_resource_create(client,
              &wl_callback_interface, wl_callback_interface.version, callback);
          if (!callback_resource) {
            wl_client_post_no_memory(client);
            return;
          }
          core::server::get().surface_map.at(resource)->set_frame_callback(
              callback_resource);
        },
    .set_opaque_region =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*region_resource*/) {
          // TODO
        },
    .set_input_region =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* region_resource) {
          const auto& self = core::server::get().surface_map.at(resource);
          auto region_it = core::server::get().region_map.find(region_resource);
          if (region_it == core::server::get().region_map.end()) {
            self->set_input_region(std::nullopt);
            return;
          }
          self->set_input_region(*region_it->second);
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource) {
          core::server::get().surface_map.at(resource)->commit();
        },
    .set_buffer_transform =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            int32_t /*transform*/) {
          // TODO
        },
    .set_buffer_scale =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            int32_t /*scale*/) {
          // TODO
        },
    .damage_buffer =
        [](wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
            int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
          // TODO
        },
    .offset =
        [](wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*dx*/,
            int32_t /*dy*/) {
          // TODO
        },
    .get_release =
        [](wl_client* client, wl_resource* resource, uint32_t callback) {
          wl_resource* callback_resource = wl_resource_create(client,
              &wl_callback_interface, wl_callback_interface.version, callback);
          if (!callback_resource) {
            wl_client_post_no_memory(client);
            return;
          }
          core::server::get().surface_map.at(resource)->set_release_callback(
              callback_resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().surface_map.erase(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_surface_interface, wl_surface_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  core::server::get().surface_map.emplace(
      resource, std::make_shared<core::Surface>(resource));
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
}
}  // namespace yabe::proto::wayland::surface
