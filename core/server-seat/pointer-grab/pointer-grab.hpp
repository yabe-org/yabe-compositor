#pragma once

#include "common/common.hpp"

namespace yabe::core::server_seat::pointer_grab {
class PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(PointerGrab);

  virtual void cleanup()                                = 0;
  virtual void move_relative(float dx, float dy)        = 0;
  virtual void button(uint32_t button, bool is_pressed) = 0;

 protected:
  explicit PointerGrab() = default;
  ~PointerGrab()         = default;
};
}  // namespace yabe::core::server_seat::pointer_grab
