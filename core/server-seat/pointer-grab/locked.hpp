#pragma once

#include <memory>

#include "common/common.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "protocols/wayland/pointer-constraints/locked-pointer.hpp"

namespace yabe::core::server_seat::pointer_grab {
// Activated when:
//   - Surface has LockedPointer and its SurfaceNode is activated
//   - LockedPointer is created for the pointer-focused Surface
class LockedPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(LockedPointerGrab);

  static LockedPointerGrab* start(
      const std::shared_ptr<scene::SurfaceNode>& locked_surface_node);
  [[nodiscard]] static bool can_start_with(
      const std::shared_ptr<scene::SurfaceNode>& node);
  [[nodiscard]] static bool is_locking(const std::shared_ptr<
      proto::wayland::pointer_constraints::locked::LockedPointer>&
          locked_pointer);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  static LockedPointerGrab instance;
  explicit LockedPointerGrab() = default;
  ~LockedPointerGrab()         = default;

  void try_send_unlocked();

  std::weak_ptr<scene::SurfaceNode> locked_surface_node_;
};
}  // namespace yabe::core::server_seat::pointer_grab
