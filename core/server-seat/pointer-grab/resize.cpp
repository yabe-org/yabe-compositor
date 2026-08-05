#include "resize.hpp"

#include <linux/input-event-codes.h>

#include <memory>

#include "common/unit.hpp"
#include "core/server.hpp"
#include "protocols/xdg-shell/xdg-toplevel.hpp"
#include "xdg-shell-protocol.h"

namespace yabe::core::server_seat::pointer_grab {
ResizePointerGrab* ResizePointerGrab::start(
    std::weak_ptr<proto::xdg_shell::xdg_toplevel::XdgToplevel>&& toplevel,
    uint32_t                                                     edges) {
  static ResizePointerGrab instance;
  server::get().seat.pointer.unfocus_surface();
  instance.resizing_toplevel_ = std::move(toplevel);
  instance.edges_             = edges;
  instance.move_amount_x_     = 0.F;
  instance.move_amount_y_     = 0.F;

  const auto xdg_toplevel        = instance.resizing_toplevel_.lock();
  const auto xdg_surface         = xdg_toplevel->xdg_surface().lock();
  const auto locked_toplevel     = xdg_toplevel->surface_node().lock();
  instance.initial_width_        = xdg_surface->width();
  instance.initial_height_       = xdg_surface->height();
  instance.initial_toplevel_pos_ = locked_toplevel->geometry().pos();
  return &instance;
}

void ResizePointerGrab::cleanup() {
  server::get().seat.pointer.focus_pointed_surface();
}

// FIXME: The toplevel is placed and rotated so that it looks at the camera
// so resizing behavior looks not the same as the movement of the cursor
// if resize amount becomes large
void ResizePointerGrab::move_relative(float dx, float dy) {
  server::get().ray_cursor.move_relative(-dy, -dx);

  // dx, dy are in OpenGL coordinate (+Y is upper)
  // and adjusted as a polar angles
  if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_RIGHT) {
    this->move_amount_x_ += dx;
  } else if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_LEFT) {
    this->move_amount_x_ -= dx;
  }
  if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_TOP) {
    this->move_amount_y_ += dy;
  } else if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM) {
    this->move_amount_y_ -= dy;
  }

  const auto xdg_toplevel = this->resizing_toplevel_.lock();
  if (!xdg_toplevel) {
    server::get().seat.pointer.end_grab();
    return;
  }
  const unit::WorldUnit surface_delta_width{
      std::tan(this->move_amount_x_) * scene::kSurfaceDistance};
  const unit::WorldUnit surface_delta_height{
      std::tan(this->move_amount_y_) * scene::kSurfaceDistance};
  xdg_toplevel->request_resize(
      this->initial_width_ + surface_delta_width.to_pixel(),
      this->initial_height_ + surface_delta_height.to_pixel());

  const auto surface_node = xdg_toplevel->surface_node().lock();
  if (!surface_node) {
    return;
  }
  glm::vec3 move_amount{
      *surface_delta_width / 2.F,
      *surface_delta_height / 2.F,
      0.F,
  };
  if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_LEFT) {
    move_amount.x *= -1.F;
  }
  if (this->edges_ & XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM) {
    move_amount.y *= -1.F;
  }
  surface_node->move_absolute(this->initial_toplevel_pos_ + move_amount);
  surface_node->look_at_camera();
}

void ResizePointerGrab::button(uint32_t button, bool is_pressed) {
  if (button == BTN_LEFT && !is_pressed) {
    server::get().seat.pointer.end_grab();
  }
}
}  // namespace yabe::core::server_seat::pointer_grab
