#include "workspace.hpp"

#include <wl-workspace-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "workspace-group-handle.hpp"
#include "workspace-handle.hpp"

namespace yabe::proto::wayland::workspace {
WorkspaceManager::WorkspaceManager(wl_resource* resource)
    : resource_(resource) {
}

void WorkspaceManager::add_workspace(
    int32_t index, const std::shared_ptr<WorkspaceHandle>& handle) {
  this->workspaces_.at(index) = handle;
}
void WorkspaceManager::send_done() {
  ext_workspace_manager_v1_send_done(this->resource_);
}

void WorkspaceManager::renew_workspace_state(
    int32_t previous_active_workspace, int32_t current_active_workspace) {
  if (const auto ws = this->workspaces_.at(previous_active_workspace).lock()) {
    ws->send_state(false);
  }
  if (const auto ws = this->workspaces_.at(current_active_workspace).lock()) {
    ws->send_state(true);
  }
  this->send_done();
}

namespace {
constexpr struct ext_workspace_manager_v1_interface kImpl = {
    .commit =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // this compositor only supports workspace.activate,
          // so this can be ignored...? (maybe)
        },
    .stop =
        [](wl_client* /*client*/, wl_resource* resource) {
          ext_workspace_manager_v1_send_finished(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().workspace.remove_workspace_manager(resource);
}
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(client,
      &ext_workspace_manager_v1_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);

  auto self = std::make_shared<WorkspaceManager>(resource);

  // workspace_manager.send_workspace(_group)? is sent
  // before any of their events, so pass the resource of workspace_manager.
  // sending events of workspace_group is the responsibility of
  // workspace(_group)?_handle.
  auto workspace_group = create_workspace_group_handle(client, version, self);
  if (!workspace_group) {
    return;
  }
  core::server::get().workspace.add_workspace_manager(self);
  core::server::get().workspace.for_each_scene_id(
      [self, resource, client, version, &workspace_group](
          int32_t index, bool is_active) {
        auto workspace = create_workspace_handle(
            client, version, index, is_active, resource);
        if (!workspace) {
          return;
        }
        workspace_group->workspace_enter(workspace);
        self->add_workspace(index, workspace);
      });
  self->send_done();
}
}  // namespace yabe::proto::wayland::workspace
