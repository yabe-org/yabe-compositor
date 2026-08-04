#pragma once

#include <memory>
#include <unordered_set>

#include "common/common.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"

namespace yabe::core::server_seat::pointer_grab {
class ImplicitPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(ImplicitPointerGrab);

  static ImplicitPointerGrab* start(
      const std::shared_ptr<scene::SurfaceNode>& focused_surface_node,
      uint32_t                                   button);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  std::unordered_set<uint32_t>      pressed_buttons_;
  std::weak_ptr<scene::SurfaceNode> focused_surface_node_;

  explicit ImplicitPointerGrab() = default;
  ~ImplicitPointerGrab()         = default;
};
}  // namespace yabe::core::server_seat::pointer_grab
