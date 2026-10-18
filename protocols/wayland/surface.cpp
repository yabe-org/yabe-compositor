#include "surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <optional>

#include "core/region.hpp"
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
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->attach(buffer_resource);
          }
        },
    .damage =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          // surface-local coordinates are the same as buffer coordinates
          // because buffer_scale and buffer_transform are not supported (TODO?)
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->damage(x, y, width, height);
          }
        },
    .frame =
        [](wl_client* client, wl_resource* resource, uint32_t callback) {
          wl_resource* callback_resource = wl_resource_create(client,
              &wl_callback_interface, wl_callback_interface.version, callback);
          if (!callback_resource) {
            wl_client_post_no_memory(client);
            return;
          }
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->set_frame_callback(callback_resource);
          }
        },
    .set_opaque_region =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*region_resource*/) {
          // TODO
        },
    .set_input_region =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* region_resource) {
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          const auto region =
              core::server::get().resources.get<core::Region>(region_resource);
          if (!region) {
            self->set_input_region(std::nullopt);
            return;
          }
          self->set_input_region(*region);
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->commit();
          }
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
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->damage(x, y, width, height);
          }
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
          const auto self =
              core::server::get().resources.get<core::Surface>(resource);
          if (self) {
            self->set_release_callback(callback_resource);
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_surface_interface, wl_surface_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  core::server::get().resources.add(
      resource, std::make_shared<core::Surface>(resource));
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
}
}  // namespace yabe::proto::wayland::surface
