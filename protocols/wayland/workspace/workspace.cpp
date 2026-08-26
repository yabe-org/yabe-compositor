#include "workspace.hpp"

#include <wl-workspace-protocol.h>

#include "core/server.hpp"
#include "workspace-group-handle.hpp"
#include "workspace-handle.hpp"

namespace yabe::proto::wayland::workspace {
namespace {
constexpr struct ext_workspace_manager_v1_interface kImpl = {
    .commit =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // this compositor only supports workspace.activate,
          // so this can be ignored...? (maybe)
        },
    .stop =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // This requests to stop sending events about workspace groups,
          // which are not supported, so ignore this request
        },
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(client,
      &ext_workspace_manager_v1_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);

  // workspace_manager.send_workspace(_group)? is sent
  // before any of their events, so pass the resource of workspace_manager.
  // sending events of workspace_group is the responsibility of
  // workspace(_group)?_handle.
  auto workspace_group =
      create_workspace_group_handle(client, version, resource);
  if (!workspace_group) {
    return;
  }
  core::server::get().workspace.for_each_scene_id(
      [resource, client, version, &workspace_group](
          int32_t index, bool is_active) {
        const auto workspace = create_workspace_handle(
            client, version, index, is_active, resource);
        if (!workspace) {
          return;
        }
        workspace_group->workspace_enter(workspace);
      });
  ext_workspace_manager_v1_send_done(resource);
}
}  // namespace yabe::proto::wayland::workspace
