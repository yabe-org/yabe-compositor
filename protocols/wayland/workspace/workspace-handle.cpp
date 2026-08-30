#include "workspace-handle.hpp"

#include <wayland-server-core.h>
#include <wl-workspace-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "workspace-group-handle.hpp"

namespace yabe::proto::wayland::workspace {
WorkspaceHandle::WorkspaceHandle(wl_resource* resource, int32_t index)
    : index_(index), resource_(resource) {
}
void WorkspaceHandle::activate() const {
  core::server::get().workspace.set_current_scene_index(this->index_);
}

namespace {
constexpr struct ext_workspace_handle_v1_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .activate =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<WorkspaceHandle>(resource);
          if (self) {
            self->activate();
          }
        },
    .deactivate =
        [](wl_client* client, wl_resource* /*resource*/) {
          wl_client_post_implementation_error(
              client, "workspace_handle.deactivate is not supported");
        },
    .assign =
        [](wl_client* client, wl_resource* /*resource*/,
            wl_resource* /*output*/) {
          wl_client_post_implementation_error(
              client, "workspace_handle.assign is not supported");
        },
    .remove =
        [](wl_client* client, wl_resource* /*resource*/
        ) {
          wl_client_post_implementation_error(
              client, "workspace_handle.remove is not supported");
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

std::shared_ptr<WorkspaceHandle> create_workspace_handle(wl_client* client,
    uint32_t version, int32_t workspace_index, bool is_active,
    wl_resource* workspace_manager) {
  wl_resource* resource = wl_resource_create(
      client, &ext_workspace_handle_v1_interface, static_cast<int>(version), 0);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return {};
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);

  const auto self =
      std::make_shared<WorkspaceHandle>(resource, workspace_index);
  core::server::get().resources.add(resource, self);

  // NOLINTNEXTLINE(readability-suspicious-call-argument)
  ext_workspace_manager_v1_send_workspace(workspace_manager, resource);
  // according to the protocol, id and coordinates are optional
  ext_workspace_handle_v1_send_name(
      resource, std::to_string(workspace_index).c_str());
  ext_workspace_handle_v1_send_state(
      resource, is_active ? EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE : 0);
  ext_workspace_handle_v1_send_capabilities(
      resource, EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_ACTIVATE);

  return self;
}
}  // namespace yabe::proto::wayland::workspace
