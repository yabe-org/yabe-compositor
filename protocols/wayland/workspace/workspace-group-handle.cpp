#include "workspace-group-handle.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wl-workspace-protocol.h>

#include <memory>

#include "core/server.hpp"
#include "protocols/wayland/workspace/workspace-handle.hpp"

namespace yabe::proto::wayland::workspace {
WorkspaceGroupHandle::WorkspaceGroupHandle(wl_resource* resource)
    : resource_(resource) {
}
void WorkspaceGroupHandle::output_enter(wl_resource* output) const {
  ext_workspace_group_handle_v1_send_output_enter(this->resource_, output);
}
void WorkspaceGroupHandle::workspace_enter(
    const std::shared_ptr<WorkspaceHandle>& workspace) const {
  ext_workspace_group_handle_v1_send_workspace_enter(
      this->resource_, workspace->resource());
}

namespace {
constexpr struct ext_workspace_group_handle_v1_interface kImpl = {
    .create_workspace =
        [](wl_client* client, wl_resource* /*resource*/,
            const char* /*workspace*/) {
          wl_client_post_implementation_error(client,
              "workspace_group_handle.create_workspace is not supported");
        },
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

std::shared_ptr<WorkspaceGroupHandle> create_workspace_group_handle(
    wl_client* client, uint32_t version,
    const std::shared_ptr<WorkspaceManager>& manager) {
  wl_resource* resource = wl_resource_create(client,
      &ext_workspace_group_handle_v1_interface, static_cast<int>(version), 0);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return {};
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);

  const auto self = std::make_shared<WorkspaceGroupHandle>(resource);
  core::server::get().resources.add(resource, self);

  // NOLINTNEXTLINE(readability-suspicious-call-argument)
  ext_workspace_manager_v1_send_workspace_group(manager->resource(), resource);
  const auto output = core::server::get().output_map.find(client);
  if (output != core::server::get().output_map.end()) {
    self->output_enter(output->second);
  }
  return self;
}
}  // namespace yabe::proto::wayland::workspace
