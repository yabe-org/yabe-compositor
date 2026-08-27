#include "workspace.hpp"

#include "common/common.hpp"

namespace yabe::core {
void Workspace::set_current_scene_index(int32_t index) {
  if (index < 0 || index >= static_cast<int32_t>(scenes_.size())) {
    LOG_ERR("Scene index out of range (got {})", index);
    return;
  }
  this->current_scene_index_ = index;
  this->current_scene().focus_to_active_node();
}

void Workspace::add_scene_index(int32_t delta) {
  int32_t new_index = this->current_scene_index_ + delta;
  this->set_current_scene_index(
      std::clamp(new_index, 0, static_cast<int32_t>(scenes_.size() - 1)));
}

scene::Scene& Workspace::current_scene() {
  return this->scenes_.at(this->current_scene_index_);
}
}  // namespace yabe::core
