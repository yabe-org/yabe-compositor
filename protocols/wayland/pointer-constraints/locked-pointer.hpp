#pragma once

#include <pointer-constraints-protocol.h>
#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"

namespace yabe::core {
class Surface;
}

namespace yabe::proto::wayland::pointer_constraints::locked {
class LockedPointer {
 public:
  DISABLE_MOVE_AND_COPY(LockedPointer);
  LockedPointer(
      wl_resource* resource, zwp_pointer_constraints_v1_lifetime lifetime);
  ~LockedPointer() = default;

  void send_locked();
  void send_unlocked();
  // in order to prevent sending unlocked() on destruction
  void invalidate_resource();

  [[nodiscard]] bool can_be_locked();

 private:
  bool                                has_been_locked_ = false;
  wl_resource*                        resource_;
  zwp_pointer_constraints_v1_lifetime lifetime_;
};

void create(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<core::Surface>& surface,
    zwp_pointer_constraints_v1_lifetime   lifetime);
}  // namespace yabe::proto::wayland::pointer_constraints::locked
