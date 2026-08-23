#pragma once

#include <memory>

#include "common/common.hpp"
#include "common/unit.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "protocols/xdg/shell/xdg-toplevel.hpp"

namespace yabe::core::server_seat::pointer_grab {
class ResizePointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(ResizePointerGrab);

  // FIXME: This should not accept XdgToplevel but should accept Toplevel
  // But how can I retrieve XdgToplevel from Toplevel?
  static ResizePointerGrab* start(
      std::weak_ptr<proto::xdg::shell::xdg_toplevel::XdgToplevel>&& toplevel,
      uint32_t                                                      edges);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  explicit ResizePointerGrab() = default;
  ~ResizePointerGrab()         = default;

  uint32_t            edges_ = 0;
  common::unit::Pixel initial_width_{0};
  common::unit::Pixel initial_height_{0};
  glm::vec3           initial_toplevel_pos_{0.F};

  float move_amount_x_ = 0.F;
  float move_amount_y_ = 0.F;

  std::weak_ptr<proto::xdg::shell::xdg_toplevel::XdgToplevel>
      resizing_toplevel_;
};
}  // namespace yabe::core::server_seat::pointer_grab
