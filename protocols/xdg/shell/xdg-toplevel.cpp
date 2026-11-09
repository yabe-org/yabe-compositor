#include "xdg-toplevel.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-protocol.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "common/edge.hpp"
#include "core/scene/decoration-node.hpp"
#include "core/scene/resize-handle-node.hpp"
#include "core/scene/scene.hpp"
#include "core/server-seat/pointer-grab/move.hpp"
#include "core/server-seat/pointer-grab/resize.hpp"
#include "core/server.hpp"
#include "core/toplevel.hpp"
#include "protocols/xdg/shell/xdg-surface.hpp"

namespace yabe::proto::xdg::shell::xdg_toplevel {
XdgToplevel::XdgToplevel(
    wl_resource* resource, std::weak_ptr<xdg_surface::XdgSurface>&& surface)
    : resource_(resource)
    , decoration_(std::make_shared<core::scene::DecorationNode>())
    , xdg_surface_(std::move(surface)) {
  LOG_DBG("xdg_toplevel#{}: created", wl_resource_get_id(resource));
}
XdgToplevel::~XdgToplevel() {
  LOG_DBG("xdg_toplevel#{}: destroyed", wl_resource_get_id(this->resource_));
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  const auto surface = xdg_surface->wl_surface().lock();
  if (!surface) {
    return;
  }
  surface->unmap();
}

void XdgToplevel::commit() {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  xdg_surface->commit();
  if (this->is_initial_commit_sent_) {
    return;
  }
  this->is_initial_commit_sent_ = true;
  this->send_configure(0, 0);
}

void XdgToplevel::map() {
  const auto surface = this->xdg_surface_.lock()->wl_surface().lock();
  if (!surface) {
    return;
  }
  surface->map();
  const auto node = surface->surface_node().value();
  this->resize_handle_ =
      std::make_shared<core::scene::ResizeHandleNode>(this->shared_from_this());
  const auto lock = core::scene::Scene::mutex.lock();
  node->add_child(this->decoration_);
  node->add_child(this->resize_handle_);
}
void XdgToplevel::set_activated(bool is_activated) {
  this->is_activated_ = is_activated;
  this->send_configure(0, 0);
}
std::shared_ptr<core::Toplevel> XdgToplevel::get_toplevel() {
  return this->shared_from_this();
}

std::weak_ptr<core::Surface> XdgToplevel::surface() {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return {};
  }
  return xdg_surface->wl_surface();
}

void XdgToplevel::set_parent(
    std::optional<std::shared_ptr<XdgToplevel>> new_parent) {
  if (new_parent.value_or(nullptr) == this->parent_.lock()) {
    // note that it also returns if both are null
    return;
  }
  const auto node = this->surface_node().lock();
  if (!node) {
    return;
  }

  if (!new_parent) {
    node->detach_from_parent();
    this->parent_.reset();
    return;
  }
  const auto new_parent_node = new_parent.value()->surface_node().lock();
  if (!new_parent_node) {
    node->detach_from_parent();
    this->parent_.reset();
    return;
  }
  this->parent_                    = *new_parent;
  core::scene::Scene* scene        = nullptr;
  core::scene::Scene* parent_scene = nullptr;
  {
    const auto lock = core::scene::Scene::mutex.lock();
    scene           = node->owner();
    parent_scene    = new_parent_node->owner();
  }
  scene->erase(node);

  {
    const auto lock             = core::scene::Scene::mutex.lock();
    // reset pos&rot so that parent can decide them of the child
    node->geometry().pos()      = glm::vec3(0.F);
    node->geometry().rotation() = glm::quat();
    new_parent_node->add_child(node);
  }
  // if parent is keyboard-focused, newly added child should be focused now
  const auto keyboard_focused_surface =
      core::server::get().seat.keyboard.get_focused_surface().lock();
  const auto parent_surface = new_parent.value()->surface().lock();
  if (keyboard_focused_surface != parent_surface) {
    return;
  }
  parent_scene->set_active_node(node);
}

void XdgToplevel::start_move() {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  const auto surface = xdg_surface->wl_surface().lock();
  if (!surface) {
    return;
  }
  const auto surface_node = surface->surface_node();
  if (!surface_node.has_value()) {
    return;
  }
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(
          surface_node.value()));
}
void XdgToplevel::start_resize(uint32_t edges) {
  common::Edge resize_edge;
  if (edges & XDG_TOPLEVEL_RESIZE_EDGE_RIGHT) {
    resize_edge.set(common::Edge::RIGHT);
  } else if (edges & XDG_TOPLEVEL_RESIZE_EDGE_LEFT) {
    resize_edge.set(common::Edge::LEFT);
  }
  if (edges & XDG_TOPLEVEL_RESIZE_EDGE_TOP) {
    resize_edge.set(common::Edge::TOP);
  } else if (edges & XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM) {
    resize_edge.set(common::Edge::BOTTOM);
  }

  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::ResizePointerGrab::start(
          this->shared_from_this(), resize_edge));
}

void XdgToplevel::request_resize(
    common::unit::Pixel width, common::unit::Pixel height) {
  this->send_configure(*width, *height);
}

void XdgToplevel::send_configure(int32_t width, int32_t height) {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return;
  }
  wl_array states;
  wl_array_init(&states);
  if (this->is_activated_) {
    auto* state =
        static_cast<uint32_t*>(wl_array_add(&states, sizeof(uint32_t)));
    if (state) {
      *state = XDG_TOPLEVEL_STATE_ACTIVATED;
    }
  }
  xdg_toplevel_send_configure(this->resource_, width, height, &states);
  wl_array_release(&states);
  xdg_surface->configure();
}

std::weak_ptr<core::scene::SurfaceNode> XdgToplevel::surface_node() {
  const auto xdg_surface = this->xdg_surface_.lock();
  if (!xdg_surface) {
    return {};
  }
  const auto surface = xdg_surface->wl_surface().lock();
  if (!surface) {
    return {};
  }
  const auto surface_node = surface->surface_node();
  if (!surface_node.has_value()) {
    return {};
  }
  return surface_node.value();
}
bool XdgToplevel::is_ancestor_of(
    const std::shared_ptr<XdgToplevel>& other) const {
  for (auto ancestor = other; ancestor; ancestor = ancestor->parent_.lock()) {
    if (ancestor.get() == this) {
      return true;
    }
  }
  return false;
}

namespace {
constexpr struct xdg_toplevel_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_parent =
        [](wl_client* client, wl_resource* resource,
            wl_resource* parent_resource) {
          const auto self =
              core::server::get().resources.get<XdgToplevel>(resource);
          std::optional<std::shared_ptr<XdgToplevel>> parent = std::nullopt;
          if (parent_resource) {
            auto ptr =
                core::server::get().resources.get<XdgToplevel>(parent_resource);
            if (!ptr) {
              wl_client_post_implementation_error(
                  client, "parent is non-null resource but not found");
              return;
            }
            if (self->is_ancestor_of(ptr)) {
              wl_resource_post_error(resource,
                  XDG_TOPLEVEL_ERROR_INVALID_PARENT,
                  "requested toplevel is same as or the ancestor of "
                  "new parent");
              return;
            }
            parent = ptr;
          }
          self->set_parent(parent);
        },
    .set_title =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            const char* /*title*/) {
          // TODO
        },
    .set_app_id =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            const char* /*app_id*/) {
          // TODO
        },
    .show_window_menu =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*seat*/, uint32_t /*serial*/, int32_t /*x*/,
            int32_t /*y*/) {
          // TODO
        },
    .move =
        [](wl_client* /*client*/, wl_resource* resource, wl_resource* /*seat*/,
            uint32_t /*serial*/) {
          core::server::get()
              .resources.get<XdgToplevel>(resource)
              ->start_move();
        },
    .resize =
        [](wl_client* /*client*/, wl_resource* resource, wl_resource* /*seat*/,
            uint32_t /*serial*/, uint32_t      edges) {
          core::server::get()
              .resources.get<XdgToplevel>(resource)
              ->start_resize(edges);
        },
    .set_max_size =
        [](wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*width*/,
            int32_t /*height*/) {
          // TODO
        },
    .set_min_size =
        [](wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*width*/,
            int32_t /*height*/) {
          // TODO
        },
    .set_maximized =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
    .unset_maximized =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
    .set_fullscreen =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*output*/) {
          // TODO
        },
    .unset_fullscreen =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
    .set_minimized =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

wl_resource* create(wl_client* client, int version, uint32_t id,
    std::weak_ptr<xdg_surface::XdgSurface> xdg_surface) {
  const auto locked_xdg_surface = xdg_surface.lock();
  if (!locked_xdg_surface) {
    return nullptr;
  }
  const auto surface = locked_xdg_surface->wl_surface().lock();
  if (!surface) {
    return nullptr;
  }
  wl_resource* resource =
      wl_resource_create(client, &xdg_toplevel_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto self = std::make_shared<XdgToplevel>(resource, std::move(xdg_surface));
  auto toplevel = std::static_pointer_cast<core::Toplevel>(self);
  core::server::get().resources.add(resource, self);
  // first set_role(), then map() because the SurfaceNode may be
  // focused and activated if there is no active node in the scene,
  // and activation uses SurfaceRole::get_toplevel()
  surface->set_role(self);
  self->map();
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  return resource;
}
}  // namespace yabe::proto::xdg::shell::xdg_toplevel
