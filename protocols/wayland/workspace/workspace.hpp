#pragma once

#include <wayland-server-core.h>

#include <array>
#include <memory>

#include "common/common.hpp"
#include "core/workspace.hpp"
#include "workspace-handle.hpp"

namespace yabe::proto::wayland::workspace {
class WorkspaceManager {
 public:
  DISABLE_MOVE_AND_COPY(WorkspaceManager);
  explicit WorkspaceManager(wl_resource* resource);
  ~WorkspaceManager() = default;

  void send_done();

  void add_workspace(
      int32_t index, const std::shared_ptr<WorkspaceHandle>& handle);
  void renew_workspace_state(
      int32_t previous_active_workspace, int32_t current_active_workspace);

  [[nodiscard]] wl_resource* resource() const {
    return resource_;
  }

 private:
  wl_resource*                                                   resource_;
  std::array<std::weak_ptr<WorkspaceHandle>, core::kSceneLength> workspaces_;
};

void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}  // namespace yabe::proto::wayland::workspace
