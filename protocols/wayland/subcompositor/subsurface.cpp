#include "subsurface.hpp"

#include <wayland-server-protocol-core.h>

#include "common/common.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server.hpp"
#include "core/toplevel.hpp"

namespace yabe::proto::wayland::subcompositor::subsurface {
Subsurface::Subsurface(wl_resource* resource,
    std::shared_ptr<core::Surface>& surface,
    std::shared_ptr<core::Surface>& parent_surface)
    : resource_(resource), surface_(surface), parent_surface_(parent_surface) {
  LOG_DBG("wl_subsurface#{}: created", wl_resource_get_id(resource));
}
Subsurface::~Subsurface() {
  LOG_DBG("wl_subsurface#{}: destroyed", wl_resource_get_id(this->resource_));
  const auto surface = this->surface_.lock();
  if (surface) {
    surface->unmap();
  }
}

void Subsurface::commit() {
  auto surface = this->surface_.lock();
  if (!surface) {
    return;
  }
  if (!surface->surface_node().has_value()) {
    // subsurface should be mapped when these are met:
    // - parent is mapped
    auto parent_surface = this->parent_surface_.lock();
    if (!parent_surface) {
      return;
    }
    auto parent_surface_node = parent_surface->surface_node();
    if (!parent_surface_node.has_value()) {
      return;
    }
    // - wl_surface of subsurface has a buffer
    if (!surface->has_buffer()) {
      return;
    }
    // then, map subsurface if it is not mapped yet
    surface->map(parent_surface_node.value(),
        core::scene::SurfaceNode::Anchor::TOP_LEFT);
  }
  this->update_position();
}
std::shared_ptr<core::Toplevel> Subsurface::get_toplevel() {
  auto parent_surface = this->parent_surface_.lock();
  if (!parent_surface) {
    return nullptr;
  }
  return parent_surface->get_toplevel();
}

void Subsurface::set_position(int32_t x, int32_t y) {
  this->x_ = x;
  this->y_ = y;
  this->update_position();
}

void Subsurface::update_position() {
  auto surface = this->surface_.lock();
  if (!surface) {
    return;
  }
  auto surface_node = surface->surface_node();
  if (!surface_node.has_value()) {
    return;
  }
  auto parent_surface = this->parent_surface_.lock();
  if (!parent_surface) {
    return;
  }
  auto parent_surface_node = parent_surface->surface_node();
  if (!parent_surface_node.has_value()) {
    return;
  }
  surface_node.value()->move_absolute(
      {*this->x_.to_world_unit(), -*this->y_.to_world_unit(), 0.F});
}

namespace {
constexpr struct wl_subsurface_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_position =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y) {
          core::server::get().subsurface_map.at(resource)->set_position(x, y);
        },
    .place_above =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*sibling*/) {
          // TODO
        },
    .place_below =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*sibling*/) {
          // TODO
        },
    .set_sync =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
    .set_desync =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().subsurface_map.erase(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id,
    std::shared_ptr<core::Surface>& surface,
    std::shared_ptr<core::Surface>& parent_surface) {
  wl_resource* resource = wl_resource_create(
      client, &wl_subsurface_interface, wl_subsurface_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto subsurface =
      std::make_shared<Subsurface>(resource, surface, parent_surface);
  surface->set_role(subsurface);
  core::server::get().subsurface_map.emplace(resource, std::move(subsurface));
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
}
}  // namespace yabe::proto::wayland::subcompositor::subsurface
