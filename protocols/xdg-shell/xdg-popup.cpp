#include "xdg-popup.hpp"

#include <optional>

#include "common/common.hpp"
#include "common/geometry.hpp"
#include "core/server.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_popup {
XdgPopup::XdgPopup(wl_resource*               resource,
    std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
    std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
    xdg_positioner::XdgPositioner*            positioner)
    : resource_(resource)
    , popup_surface_(popup_surface)
    , parent_surface_(parent_surface)
    , positioner_(std::make_unique<xdg_positioner::XdgPositioner>(positioner)) {
  const auto parent_wl_surface = parent_surface->wl_surface().lock();
  if (!parent_wl_surface) {
    wl_client_post_implementation_error(wl_resource_get_client(resource),
        "xdg_popup requested for unknown parent surface");
    return;
  }
  const auto parent_surface_node = parent_wl_surface->surface_node();
  if (!parent_surface_node.has_value()) {
    return;
  }
  // popup surface (weak_ptr) is validated in create() function
  const auto popup_wl_surface = popup_surface->wl_surface().lock();
  popup_wl_surface->map(parent_surface_node.value());

  this->reposition(std::nullopt);
}

void XdgPopup::commit() {
  if (this->is_initial_commit_sent_) {
    return;
  }
  const auto xdg_surface = this->popup_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  this->is_initial_commit_sent_ = true;
  xdg_surface->configure();
}
void XdgPopup::set_activated(bool /*is_activated*/) {
  // todo?
}

void XdgPopup::reposition(std::optional<uint32_t> token) {
  const auto popup_surface = this->popup_surface_.lock();
  if (!popup_surface) {
    return;
  }
  const auto popup_wl_surface = popup_surface->wl_surface().lock();
  if (!popup_wl_surface) {
    return;
  }
  const auto popup_surface_node = popup_wl_surface->surface_node();
  if (!popup_surface_node.has_value()) {
    return;
  }

  const auto parent_surface = this->parent_surface_.lock();
  if (!parent_surface) {
    return;
  }
  const auto parent_wl_surface = parent_surface->wl_surface().lock();
  if (!parent_wl_surface) {
    return;
  }
  const auto parent_surface_node = parent_wl_surface->surface_node();
  if (!parent_surface_node.has_value()) {
    return;
  }

  auto       positioner_result = this->positioner_->calculate_pos();
  const auto parent_pos =
      parent_surface_node.value()->geometry().cartesian().pos;
  const auto parent_top_left =
      parent_pos - glm::vec3{
                       *parent_surface->width().to_world_unit() / 2.F,
                       *parent_surface->height().to_world_unit() / 2.F * -1.F,
                       0.F,
                   };
  const auto popup_top_left =
      parent_top_left + glm::vec3{
                            *positioner_result.x.to_world_unit(),
                            *positioner_result.y.to_world_unit() * -1.F,
                            0.F,
                        };
  auto popup_new_pos =
      popup_top_left + glm::vec3{
                           +(*positioner_result.width.to_world_unit() / 2.F),
                           -(*positioner_result.height.to_world_unit() / 2.F),
                           0.F,
                       };

  // move popup little align with parent's normal vector to avoid z-fighting
  // maybe SurfaceNode should treat parent position?
  // if so, it should set anchor (default center but top-left for popup) (TODO)
  const auto parent_normal = glm::normalize(
      glm::vec3{parent_surface_node.value()->geometry().model_mat()[2]});
  popup_new_pos += parent_normal * 0.01F;
  popup_surface_node.value()->move_absolute(popup_new_pos);

  xdg_popup_send_configure(this->resource_, *positioner_result.x,
      *positioner_result.y, *positioner_result.width,
      *positioner_result.height);
  popup_surface->configure();

  if (token.has_value()) {
    xdg_popup_send_repositioned(this->resource_, token.value());
  }
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void grab(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/) {
  // TODO
}
void reposition(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*positioner*/, uint32_t /*token*/) {
  // TODO
}
constexpr struct xdg_popup_interface kImpl = {
    .destroy    = destroy,
    .grab       = grab,
    .reposition = reposition,
};
}  // namespace

void create(wl_client* client, uint32_t id,
    std::shared_ptr<xdg_surface::XdgSurface>& popup_surface,
    std::shared_ptr<xdg_surface::XdgSurface>& parent_surface,
    xdg_positioner::XdgPositioner*            positioner) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_popup_interface, xdg_popup_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  const auto popup_wl_surface = popup_surface->wl_surface().lock();
  if (!popup_wl_surface) {
    wl_client_post_implementation_error(wl_resource_get_client(resource),
        "xdg_popup requested for unknown popup surface");
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);

  auto popup = std::make_shared<XdgPopup>(
      resource, popup_surface, parent_surface, positioner);
  popup_wl_surface->set_role(popup);
  core::server::get().xdg_popup_map.emplace(resource, std::move(popup));
}
}  // namespace yabe::proto::xdg_shell::xdg_popup
