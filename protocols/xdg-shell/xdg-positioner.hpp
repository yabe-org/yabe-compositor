#pragma once

#include <wayland-server.h>

#include <cstdint>

#include "common/common.hpp"
#include "common/edge.hpp"
#include "common/unit.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_positioner {
// relative value from the top-left corner of the parent surface
// right and down are positive
struct PopupPosition {
  common::unit::Pixel x;
  common::unit::Pixel y;
  common::unit::Pixel width;
  common::unit::Pixel height;
};

class XdgPositioner {
 public:
  DISABLE_MOVE_AND_COPY(XdgPositioner);
  explicit XdgPositioner(wl_resource* resource);
  explicit XdgPositioner(XdgPositioner* other);  // copy constructor
  ~XdgPositioner() = default;

  void set_size(int32_t width, int32_t height);
  void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height);
  void set_anchor(xdg_positioner_anchor anchor);
  void set_gravity(xdg_positioner_gravity gravity);
  void set_constraint_adjustment(uint32_t constraint_adjustment);
  void set_offset(int32_t x, int32_t y);
  void set_parent_size(int32_t width, int32_t height);

  PopupPosition calculate_pos();

 private:
  common::unit::Pixel width_{0};
  common::unit::Pixel height_{0};

  common::unit::Pixel anchor_x_{0};
  common::unit::Pixel anchor_y_{0};
  common::unit::Pixel anchor_width_{0};
  common::unit::Pixel anchor_height_{0};

  common::Edge anchor_;
  common::Edge gravity_;
  uint32_t constraint_adjustment_ = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_NONE;

  common::unit::Pixel offset_x_{0};
  common::unit::Pixel offset_y_{0};

  common::unit::Pixel parent_width_{0};
  common::unit::Pixel parent_height_{0};

  wl_resource* resource_;
};

void           create(wl_client* client, int version, uint32_t id);
XdgPositioner* get(wl_resource* resource);
}  // namespace yabe::proto::xdg_shell::xdg_positioner
