#pragma once

#include <wayland-server-core.h>

#include <cstdint>
#include <memory>

#include "common/common.hpp"
#include "wayland-server-protocol.h"

namespace yabe::proto::wayland::workspace {
class WorkspaceHandle;

class WorkspaceGroupHandle {
 public:
  DISABLE_MOVE_AND_COPY(WorkspaceGroupHandle);
  explicit WorkspaceGroupHandle(wl_resource* resource);
  ~WorkspaceGroupHandle() = default;

  void output_enter(wl_resource* output) const;
  void workspace_enter(const std::shared_ptr<WorkspaceHandle>& workspace) const;

  [[nodiscard]] wl_resource* resource() const {
    return resource_;
  }

 private:
  wl_resource* resource_;
};

std::shared_ptr<WorkspaceGroupHandle> create_workspace_group_handle(
    wl_client* client, uint32_t version, wl_resource* workspace_manager);
}  // namespace yabe::proto::wayland::workspace
