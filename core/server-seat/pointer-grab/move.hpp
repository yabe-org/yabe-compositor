#pragma once

#include <memory>

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"
#include "core/surface.hpp"

namespace yabe::core::server_seat::pointer_grab {
class MovePointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(MovePointerGrab);

  static MovePointerGrab* start(std::weak_ptr<Surface>&& surface);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  explicit MovePointerGrab() = default;
  ~MovePointerGrab()         = default;

  std::weak_ptr<Surface> moving_surface_;
};
}  // namespace yabe::core::server_seat::pointer_grab
