#include "layer-surface.hpp"

#include <wlr-layer-shell-protocol.h>

#include "common/box.hpp"
#include "common/common.hpp"
#include "common/edge.hpp"
#include "common/margin.hpp"
#include "common/unit.hpp"
#include "core/server.hpp"
#include "protocols/wayland/output.hpp"

namespace yabe::proto::wlr::layer_shell::layer_surface {
LayerSurface::LayerSurface(wl_resource* resource,
    std::shared_ptr<core::Surface>& wl_surface, zwlr_layer_shell_v1_layer layer,
    std::string&& name_space)
    : resource_(resource)
    , wl_surface_(wl_surface)
    , layer_(layer)
    , name_space_(std::move(name_space)) {
  LOG_DBG("layer_surface#{}: created", wl_resource_get_id(resource));
}
LayerSurface::~LayerSurface() {
  LOG_DBG("layer_surface#{}: destroyed", wl_resource_get_id(this->resource_));
}

void LayerSurface::commit() {
  this->current_ = this->pending_;
  core::server::get().layer_manager.reconfigure(this->current_.layer);
}
[[nodiscard]] std::shared_ptr<core::Toplevel> LayerSurface::get_toplevel() {
  return {};
}

void LayerSurface::reconfigure(common::Box2D& available_area) {
  common::Box2D area;
  if (this->current_.exclusive_zone == -1) {
    area = common::Box2D{
        .x{0},
        .y{0},
        .width{wayland::output::get_viewport_width()},
        .height{wayland::output::get_viewport_height()},
    };
  } else {
    area = available_area;
  }
  common::Box2D result{
      .x{area.x},
      .y{area.y},
      .width{this->current_.requested_width},
      .height{this->current_.requested_height},
  };

  if (*result.width == 0) {
    result.x += this->current_.margin.left;
    result.width =
        area.width - this->current_.margin.left - this->current_.margin.right;
  } else if (this->current_.anchor.at(common::Edge::LEFT) &&
             !this->current_.anchor.at(common::Edge::RIGHT)) {
    result.x += this->current_.margin.left;
  } else if (!this->current_.anchor.at(common::Edge::LEFT) &&
             this->current_.anchor.at(common::Edge::RIGHT)) {
    result.x += area.width - result.width - this->current_.margin.right;
  } else {
    // if LEFT and RIGHT are set, or by default, align center
    result.x +=
        this->current_.margin.left + (area.width / 2) - (result.width / 2);
  }

  if (*result.height == 0) {
    result.y += this->current_.margin.top;
    result.height =
        area.height - this->current_.margin.top - this->current_.margin.bottom;
  } else if (this->current_.anchor.at(common::Edge::TOP) &&
             !this->current_.anchor.at(common::Edge::BOTTOM)) {
    result.y += this->current_.margin.top;
  } else if (!this->current_.anchor.at(common::Edge::TOP) &&
             this->current_.anchor.at(common::Edge::BOTTOM)) {
    result.y += area.height - result.height - this->current_.margin.bottom;
  } else {
    // if TOP and BOTTOM are set, or by default, align center
    result.y +=
        this->current_.margin.top + (area.height / 2) - (result.height / 2);
  }

  // do not update available_area if this surface is not mapped (not shown)
  // to prevent other surfaces from being moved due to invisible surface
  if (this->state_ == State::MAPPED && !this->current_.exclusive_zone &&
      this->current_.exclusive_zone == -1) {
    auto zone = common::unit::Pixel{this->current_.exclusive_zone};
    switch (this->exclusive_edge_.value_or(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP)) {
      case ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP:
        available_area.y += zone + this->current_.margin.top;
        available_area.height -= zone + this->current_.margin.top;
        break;
      case ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM:
        available_area.height -= zone + this->current_.margin.bottom;
        break;
      case ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT:
        available_area.x += zone + this->current_.margin.left;
        available_area.width -= zone + this->current_.margin.left;
        break;
      case ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT:
        available_area.width -= zone + this->current_.margin.right;
        break;
      default:
        break;
    }
    available_area.width =
        std::max(available_area.width, common::unit::Pixel{0});
    available_area.height =
        std::max(available_area.height, common::unit::Pixel{0});
  }

  if (this->box_ == result) {
    return;
  }

  if (this->state_ == State::INITIAL) {
    this->state_ = State::SENT_FIRST_CONFIGURE;
  }
  zwlr_layer_surface_v1_send_configure(this->resource_,
      core::server::get().next_serial(), *result.width, *result.height);
  this->box_ = result;

  const auto wl_surface = this->wl_surface_.lock();
  if (wl_surface && wl_surface->has_buffer()) {
    // if this is the first time of commit() after attaching a buffer,
    // treat that is is mapped just now
    // and keyboard focusing should be performed at this time
    if (this->state_ == State::SENT_FIRST_CONFIGURE &&
        this->current_.keyboard_interactivity ==
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE) {
      core::server::get().workspace.current_scene().set_active_node({});
      core::server::get().seat.keyboard.set_focused_surface(wl_surface);
      core::server::get()
          .seat.keyboard.set_current_focused_surface_as_exclusive();
    }
    this->state_ = State::MAPPED;
  } else {
    this->state_ = State::SENT_FIRST_CONFIGURE;
  }
}

void LayerSurface::set_size(uint32_t width, uint32_t height) {
  this->pending_.requested_width  = static_cast<int32_t>(width);
  this->pending_.requested_height = static_cast<int32_t>(height);
}
void LayerSurface::set_anchor(uint32_t anchor) {
  if (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
    this->pending_.anchor.set(common::Edge::TOP);
  }
  if (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
    this->pending_.anchor.set(common::Edge::BOTTOM);
  }
  if (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) {
    this->pending_.anchor.set(common::Edge::LEFT);
  }
  if (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
    this->pending_.anchor.set(common::Edge::RIGHT);
  }
}
void LayerSurface::set_exclusive_zone(int32_t zone) {
  this->pending_.exclusive_zone = zone;
}
void LayerSurface::set_margin(
    int32_t top, int32_t right, int32_t bottom, int32_t left) {
  this->pending_.margin = common::Margin{
      .top    = common::unit::Pixel{top},
      .bottom = common::unit::Pixel{bottom},
      .left   = common::unit::Pixel{left},
      .right  = common::unit::Pixel{right},
  };
}
void LayerSurface::set_keyboard_interactivity(
    zwlr_layer_surface_v1_keyboard_interactivity interactivity) {
  this->pending_.keyboard_interactivity = interactivity;
}
void LayerSurface::set_layer(zwlr_layer_shell_v1_layer layer) {
  switch (layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      this->pending_.layer = core::Layer::BACKGROUND;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      this->pending_.layer = core::Layer::BOTTOM;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      this->pending_.layer = core::Layer::TOP;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      this->pending_.layer = core::Layer::OVERLAY;
      break;
    default:
      this->pending_.layer = core::Layer::UNSPECIFIED;
  }
}
void LayerSurface::set_exclusive_edge(zwlr_layer_surface_v1_anchor edge) {
  this->exclusive_edge_ = edge;
}

namespace {
constexpr struct zwlr_layer_surface_v1_interface kImpl = {
    .set_size =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t width,
            uint32_t height) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_size(width, height);
          }
        },
    .set_anchor =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t anchor) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_anchor(anchor);
          }
        },
    .set_exclusive_zone =
        [](wl_client* /*client*/, wl_resource* resource, int32_t zone) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_exclusive_zone(zone);
          }
        },
    .set_margin =
        [](wl_client* /*client*/, wl_resource* resource, int32_t top,
            int32_t right, int32_t bottom, int32_t left) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_margin(top, right, bottom, left);
          }
        },
    .set_keyboard_interactivity =
        [](wl_client* /*client*/, wl_resource* resource,
            uint32_t interactivity) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_keyboard_interactivity(
                static_cast<zwlr_layer_surface_v1_keyboard_interactivity>(
                    interactivity));
          }
        },
    .get_popup =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_resource* /*popup*/) {
          // TODO
        },
    .ack_configure =
        [](struct wl_client* /*client*/, struct wl_resource* /*resource*/,
            uint32_t /*serial*/) {
          // todo?
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_layer =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t layer) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_layer(
                static_cast<zwlr_layer_shell_v1_layer>(layer));
          }
        },
    .set_exclusive_edge =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t edge) {
          auto self = core::server::get().layer_manager.get(resource);
          if (self) {
            self.value()->set_exclusive_edge(
                static_cast<zwlr_layer_surface_v1_anchor>(edge));
          }
        },
};
void handle_destruction(wl_resource* resource) {
  core::server::get().layer_manager.remove(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    std::shared_ptr<core::Surface>& wl_surface, zwlr_layer_shell_v1_layer layer,
    const char* name_space) {
  wl_resource* resource =
      wl_resource_create(client, &zwlr_layer_surface_v1_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  auto self = std::make_shared<LayerSurface>(
      resource, wl_surface, layer, std::string{name_space});
  wl_surface->set_role(self);
  core::server::get().layer_manager.add(std::move(self));
}
}  // namespace yabe::proto::wlr::layer_shell::layer_surface
