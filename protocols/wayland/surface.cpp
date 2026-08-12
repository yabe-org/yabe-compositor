#include "surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <optional>

#include "core/server.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wayland::surface {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach(wl_client* /*client*/, wl_resource* resource,
    wl_resource* buffer_resource, int32_t /*dx*/, int32_t /*dy*/) {
  core::server::get().surface_map.at(resource)->attach(buffer_resource);
}
void damage(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void frame(wl_client* client, wl_resource* resource, uint32_t callback) {
  wl_resource* callback_resource = wl_resource_create(
      client, &wl_callback_interface, wl_callback_interface.version, callback);
  if (callback_resource) {
    core::server::get().surface_map.at(resource)->set_frame_callback(
        callback_resource);
  } else {
    wl_client_post_no_memory(client);
  }
}
void set_opaque_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void set_input_region(wl_client* /*client*/, wl_resource* resource,
    wl_resource* region_resource) {
  const auto& self      = core::server::get().surface_map.at(resource);
  auto        region_it = core::server::get().region_map.find(region_resource);
  if (region_it == core::server::get().region_map.end()) {
    self->set_input_region(std::nullopt);
  } else {
    self->set_input_region(*region_it->second);
  }
}
void commit(wl_client* /*client*/, wl_resource* resource) {
  core::server::get().surface_map.at(resource)->commit();
}
void set_buffer_transform(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*transform*/) {
  // TODO
}
void set_buffer_scale(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*scale*/) {
  // TODO
}
void damage_buffer(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void offset(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*dx*/,
    int32_t /*dy*/) {
  // TODO
}
void get_release(wl_client* client, wl_resource* resource, uint32_t callback) {
  wl_resource* callback_resource = wl_resource_create(
      client, &wl_callback_interface, wl_callback_interface.version, callback);
  if (callback_resource) {
    core::server::get().surface_map.at(resource)->set_release_callback(
        callback_resource);
  } else {
    wl_client_post_no_memory(client);
  }
}
constexpr struct wl_surface_interface kImpl = {
    .destroy              = destroy,
    .attach               = attach,
    .damage               = damage,
    .frame                = frame,
    .set_opaque_region    = set_opaque_region,
    .set_input_region     = set_input_region,
    .commit               = commit,
    .set_buffer_transform = set_buffer_transform,
    .set_buffer_scale     = set_buffer_scale,
    .damage_buffer        = damage_buffer,
    .offset               = offset,
    .get_release          = get_release,
};

void destroy(wl_resource* resource) {
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
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
}
}  // namespace yabe::proto::wayland::surface
