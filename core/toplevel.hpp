#pragma once

#include <memory>

#include "common/common.hpp"

namespace yabe::core {
class Surface;
class Toplevel {
 public:
  DISABLE_MOVE_AND_COPY(Toplevel);
  explicit Toplevel() = default;
  virtual ~Toplevel() = default;

  virtual void set_activated(bool is_activated)          = 0;
  [[nodiscard]] virtual std::weak_ptr<Surface> surface() = 0;
};
}  // namespace yabe::core
