#pragma once

#include <endian.h>
#include <wayland-server-core.h>

#include "common/common.hpp"

namespace yabe::proto::wlr::input_method {
class InputMethod {
 public:
  DISABLE_MOVE_AND_COPY(InputMethod);
  explicit InputMethod(wl_resource* resource);
  ~InputMethod() = default;

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }

 private:
  wl_resource* resource_;
};

void create_input_method(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::wlr::input_method
