#include "xdg-positioner.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <cassert>

#include "common/common.hpp"
#include "wayland-server-protocol.h"

namespace yabe::common {
template <>
void Edge::set(xdg_positioner_gravity value) {
  switch (value) {
    case XDG_POSITIONER_GRAVITY_NONE:
      break;
    case XDG_POSITIONER_GRAVITY_TOP:
      this->set(Pos::TOP);
      break;
    case XDG_POSITIONER_GRAVITY_BOTTOM:
      this->set(Pos::BOTTOM);
      break;
    case XDG_POSITIONER_GRAVITY_LEFT:
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_GRAVITY_RIGHT:
      this->set(Pos::RIGHT);
      break;
    case XDG_POSITIONER_GRAVITY_TOP_LEFT:
      this->set(Pos::TOP);
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_GRAVITY_TOP_RIGHT:
      this->set(Pos::TOP);
      this->set(Pos::RIGHT);
      break;
    case XDG_POSITIONER_GRAVITY_BOTTOM_LEFT:
      this->set(Pos::BOTTOM);
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT:
      this->set(Pos::BOTTOM);
      this->set(Pos::RIGHT);
      break;
  }
}
template <>
void Edge::set(xdg_positioner_anchor value) {
  switch (value) {
    case XDG_POSITIONER_ANCHOR_NONE:
      break;
    case XDG_POSITIONER_ANCHOR_TOP:
      this->set(Pos::TOP);
      break;
    case XDG_POSITIONER_ANCHOR_BOTTOM:
      this->set(Pos::BOTTOM);
      break;
    case XDG_POSITIONER_ANCHOR_LEFT:
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_ANCHOR_RIGHT:
      this->set(Pos::RIGHT);
      break;
    case XDG_POSITIONER_ANCHOR_TOP_LEFT:
      this->set(Pos::TOP);
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_ANCHOR_TOP_RIGHT:
      this->set(Pos::TOP);
      this->set(Pos::RIGHT);
      break;
    case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT:
      this->set(Pos::BOTTOM);
      this->set(Pos::LEFT);
      break;
    case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
      this->set(Pos::BOTTOM);
      this->set(Pos::RIGHT);
      break;
  }
}
}  // namespace yabe::common

namespace yabe::proto::xdg::shell::xdg_positioner {
XdgPositioner::XdgPositioner(wl_resource* resource) : resource_(resource) {
}
XdgPositioner::XdgPositioner(XdgPositioner* other) {
  this->width_                 = other->width_;
  this->height_                = other->height_;
  this->anchor_rect_           = other->anchor_rect_;
  this->anchor_                = other->anchor_;
  this->gravity_               = other->gravity_;
  this->constraint_adjustment_ = other->constraint_adjustment_;
  this->offset_x_              = other->offset_x_;
  this->offset_y_              = other->offset_y_;
  this->parent_width_          = other->parent_width_;
  this->parent_height_         = other->parent_height_;
}

void XdgPositioner::set_size(int32_t width, int32_t height) {
  this->width_  = common::unit::Pixel(width);
  this->height_ = common::unit::Pixel(height);
}
void XdgPositioner::set_anchor_rect(
    int32_t x, int32_t y, int32_t width, int32_t height) {
  this->anchor_rect_ = common::Box2D{
      .x      = common::unit::Pixel(x),
      .y      = common::unit::Pixel(y),
      .width  = common::unit::Pixel(width),
      .height = common::unit::Pixel(height),
  };
}
void XdgPositioner::set_anchor(xdg_positioner_anchor anchor) {
  this->anchor_.set(anchor);
}
void XdgPositioner::set_gravity(xdg_positioner_gravity gravity) {
  this->gravity_.set(gravity);
}
void XdgPositioner::set_constraint_adjustment(uint32_t constraint_adjustment) {
  this->constraint_adjustment_ = constraint_adjustment;
}
void XdgPositioner::set_offset(int32_t x, int32_t y) {
  this->offset_x_ = common::unit::Pixel(x);
  this->offset_y_ = common::unit::Pixel(y);
}
void XdgPositioner::set_parent_size(int32_t width, int32_t height) {
  this->parent_width_  = common::unit::Pixel(width);
  this->parent_height_ = common::unit::Pixel(height);
}

common::Box2D XdgPositioner::calculate_pos() {
  common::Box2D pos{
      .x      = this->offset_x_ + this->anchor_rect_.x,
      .y      = this->offset_y_ + this->anchor_rect_.y,
      .width  = this->width_,
      .height = this->height_,
  };

  if (this->anchor_.at(common::Edge::RIGHT)) {
    pos.x += this->anchor_rect_.width;
  } else if (!this->anchor_.at(common::Edge::LEFT)) {
    pos.x += this->anchor_rect_.width / 2;
  }
  if (this->anchor_.at(common::Edge::BOTTOM)) {
    pos.y += this->anchor_rect_.height;
  } else if (!this->anchor_.at(common::Edge::TOP)) {
    pos.y += this->anchor_rect_.height / 2;
  }

  if (this->gravity_.at(common::Edge::LEFT)) {
    pos.x -= this->width_;
  } else if (!this->gravity_.at(common::Edge::RIGHT)) {
    pos.x -= this->width_ / 2;
  }
  if (this->gravity_.at(common::Edge::TOP)) {
    pos.y -= this->height_;
  } else if (!this->gravity_.at(common::Edge::BOTTOM)) {
    pos.y -= this->height_ / 2;
  }

  return pos;
}

namespace {
constexpr struct xdg_positioner_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_size =
        [](wl_client* /*client*/, wl_resource* resource, int32_t width,
            int32_t height) {
          get(resource)->set_size(width, height);
        },
    .set_anchor_rect =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          get(resource)->set_anchor_rect(x, y, width, height);
        },
    .set_anchor =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t anchor) {
          get(resource)->set_anchor(static_cast<xdg_positioner_anchor>(anchor));
        },
    .set_gravity =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t gravity) {
          get(resource)->set_gravity(
              static_cast<xdg_positioner_gravity>(gravity));
        },
    .set_constraint_adjustment =
        [](wl_client* /*client*/, wl_resource* resource,
            uint32_t constraint_adjustment) {
          get(resource)->set_constraint_adjustment(constraint_adjustment);
        },
    .set_offset =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y) {
          get(resource)->set_offset(x, y);
        },
    .set_reactive =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // TODO
        },
    .set_parent_size =
        [](wl_client* /*client*/, wl_resource* resource, int32_t width,
            int32_t height) {
          get(resource)->set_parent_size(width, height);
        },
    .set_parent_configure =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            uint32_t /*serial*/) {
          // TODO
        },
};
void handle_destruction(wl_resource* resource) {
  delete get(resource);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_positioner_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new XdgPositioner(resource);
  wl_resource_set_implementation(resource, &kImpl, self, handle_destruction);
}
XdgPositioner* get(wl_resource* resource) {
  auto* self = wl_resource_get_user_data(resource);
  assert(wl_resource_instance_of(resource, &xdg_positioner_interface, &kImpl));
  return static_cast<XdgPositioner*>(self);
}

}  // namespace yabe::proto::xdg::shell::xdg_positioner
