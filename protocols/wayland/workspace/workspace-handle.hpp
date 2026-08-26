#pragma once

#include <wayland-server-core.h>

#include <cstdint>
#include <memory>

#include "common/common.hpp"

namespace yabe::proto::wayland::workspace {
class WorkspaceGroupHandle;

class WorkspaceHandle {
 public:
  DISABLE_MOVE_AND_COPY(WorkspaceHandle);
  explicit WorkspaceHandle(wl_resource* resource, int32_t index);
  ~WorkspaceHandle() = default;

  void activate() const;

  [[nodiscard]] wl_resource* resource() const {
    return resource_;
  }

 private:
  int32_t      index_;
  wl_resource* resource_;
};

std::shared_ptr<WorkspaceHandle> create_workspace_handle(wl_client* client,
    uint32_t version, int32_t workspace_index, bool is_active,
    wl_resource* workspace_manager);
}  // namespace yabe::proto::wayland::workspace
