#pragma once

#include "common/common.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"

namespace yabe::core::server_seat::pointer_grab {
class DefaultPointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(DefaultPointerGrab);

  static DefaultPointerGrab* start();

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  explicit DefaultPointerGrab() = default;
  ~DefaultPointerGrab()         = default;
};
}  // namespace yabe::core::server_seat::pointer_grab
