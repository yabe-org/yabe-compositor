#pragma once

#include <wayland-server.h>

#include <bitset>
#include <cstdint>

#include "common/common.hpp"
#include "common/unit.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::proto::xdg_shell::xdg_positioner {
// relative value from the top-left corner of the parent surface
// right and down are positive
struct PopupPosition {
  core::unit::Pixel x;
  core::unit::Pixel y;
  core::unit::Pixel width;
  core::unit::Pixel height;
};

class Edge {
 public:
  Edge()                       = default;
  Edge(const Edge&)            = default;
  Edge& operator=(const Edge&) = default;
  Edge(Edge&&)                 = delete;
  Edge& operator=(Edge&&)      = delete;
  ~Edge()                      = default;

  enum Pos : uint8_t {
    TOP    = 0,
    BOTTOM = 1,
    LEFT   = 2,
    RIGHT  = 3,
  };

  void set(xdg_positioner_gravity gravity);
  void set(xdg_positioner_anchor anchor);
  void set(Pos pos);
  bool at(Pos pos);

 private:
  std::bitset<4> edges_;
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
  core::unit::Pixel width_{0};
  core::unit::Pixel height_{0};

  core::unit::Pixel anchor_x_{0};
  core::unit::Pixel anchor_y_{0};
  core::unit::Pixel anchor_width_{0};
  core::unit::Pixel anchor_height_{0};

  Edge     anchor_;
  Edge     gravity_;
  uint32_t constraint_adjustment_ = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_NONE;

  core::unit::Pixel offset_x_{0};
  core::unit::Pixel offset_y_{0};

  core::unit::Pixel parent_width_{0};
  core::unit::Pixel parent_height_{0};

  wl_resource* resource_;
};

void           create(wl_client* client, int version, uint32_t id);
XdgPositioner* get(wl_resource* resource);
}  // namespace yabe::proto::xdg_shell::xdg_positioner
