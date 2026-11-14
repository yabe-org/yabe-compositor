#include "imported.hpp"

#include <wayland-server-protocol.h>
#include <xdg-foreign-protocol.h>

#include <memory>
#include <optional>

#include "core/server.hpp"
#include "core/surface.hpp"
#include "exported.hpp"
#include "protocols/xdg/shell/xdg-toplevel.hpp"

namespace yabe::proto::xdg::foreign::imported {
Imported::Imported(
    wl_resource* resource, std::weak_ptr<exported::Exported>& exported_surface)
    : exported_(exported_surface), resource_(resource) {
}
Imported::~Imported() {
  this->detach_children();
}

void Imported::set_parent_of(const std::shared_ptr<core::Surface>& child) {
  if (!child->is_role_of<shell::xdg_toplevel::XdgToplevel>()) {
    wl_resource_post_error(this->resource_,
        ZXDG_IMPORTED_V2_ERROR_INVALID_SURFACE,
        "surface role is not xdg_toplevel");
    return;
  }
  const auto exported = this->exported_.lock();
  if (!exported) {
    // invalidate_parent() is called when Exported is expired,
    // so do not send destroyed() here
    return;
  }
  const auto exported_surface = exported->surface().lock();
  if (!exported_surface) {
    // invalidate_parent() is called when the surface of Exported is destroyed
    return;
  }
  if (child->set_parent(exported_surface) ==
      core::SurfaceSetParentResult::SUCCEEDED) {
    this->children_.emplace_back(child);
  }
}

void Imported::invalidate_parent() {
  zxdg_imported_v2_send_destroyed(this->resource_);
  this->detach_children();
  this->exported_.reset();
}
void Imported::detach_children() {
  for (const auto& child_weak : this->children_) {
    const auto child    = child_weak.lock();
    const auto exported = this->exported_.lock();
    if (!child || !exported) {
      continue;
    }
    const auto parent = child->parent();
    // ignore if parent surface != exported surface
    if (parent.owner_before(exported->surface()) ||
        exported->surface().owner_before(parent)) {
      continue;
    }
    child->set_parent(std::nullopt);
  }
  this->children_.clear();
}
void Imported::try_remove_from_exported() {
  if (const auto exported = this->exported_.lock()) {
    exported->remove_imported(this->shared_from_this());
  }
}

namespace {
constexpr struct zxdg_imported_v2_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_parent_of =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* child_resource) {
          const auto child =
              core::server::get().resources.get<core::Surface>(child_resource);
          if (child) {
            core::server::get()
                .resources.get<Imported>(resource)
                ->set_parent_of(child);
          }
        },
};
void handle_destruction(wl_resource* resource) {
  const auto self = core::server::get().resources.get<Imported>(resource);
  if (!self) {
    return;
  }
  self->try_remove_from_exported();
  core::server::get().resources.remove(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    std::weak_ptr<exported::Exported>& exported) {
  wl_resource* resource =
      wl_resource_create(client, &zxdg_imported_v2_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  auto self = std::make_shared<Imported>(resource, exported);
  core::server::get().resources.add(resource, self);
  if (const auto e = exported.lock()) {
    // even Exported is valid, its surface migit be invalid
    if (e->surface().lock()) {
      e->add_imported(self);
      return;
    }
  }
  zxdg_imported_v2_send_destroyed(resource);
}
}  // namespace yabe::proto::xdg::foreign::imported
