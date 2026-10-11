#include "workspace.hpp"

#include <algorithm>
#include <memory>

#include "common/common.hpp"
#include "protocols/wayland/workspace/workspace.hpp"

namespace yabe::core {
void Workspace::set_current_scene_index(int32_t index) {
  if (index < 0 || index >= static_cast<int32_t>(scenes_.size())) {
    LOG_ERR("Scene index out of range (got {})", index);
    return;
  }
  if (this->current_scene_index_ == index) {
    return;
  }
  const auto previous        = this->current_scene_index_;
  this->current_scene_index_ = index;
  this->current_scene().focus_to_active_node();
  for (auto& manager : this->workspace_managers_) {
    manager->renew_workspace_state(previous, index);
  }
}

void Workspace::add_scene_index(int32_t delta) {
  int32_t new_index = this->current_scene_index_ + delta;
  this->set_current_scene_index(
      std::clamp(new_index, 0, static_cast<int32_t>(scenes_.size() - 1)));
}

scene::Scene& Workspace::current_scene() {
  return this->scenes_.at(this->current_scene_index_);
}

void Workspace::add_workspace_manager(
    std::shared_ptr<proto::wayland::workspace::WorkspaceManager> manager) {
  this->workspace_managers_.emplace_back(std::move(manager));
}
void Workspace::remove_workspace_manager(wl_resource* manager) {
  auto it = std::ranges::find_if(this->workspace_managers_,
      [manager](
          const std::shared_ptr<proto::wayland::workspace::WorkspaceManager>&
              e) {
        return e->resource() == manager;
      });
  if (it == this->workspace_managers_.end()) {
    return;
  }
  this->workspace_managers_.erase(it);
}
}  // namespace yabe::core
