#pragma once

#include <list>
#include <memory>

#include "common/common.hpp"
#include "core/scene/scene.hpp"

namespace yabe::proto::wayland::workspace {
class WorkspaceManager;
}

namespace yabe::core {
constexpr uint32_t kSceneLength = 10;
class Workspace {
 public:
  DISABLE_MOVE_AND_COPY(Workspace);
  Workspace()  = default;
  ~Workspace() = default;

  // @param index must be [0, 9]
  void                        set_current_scene_index(int32_t index);
  void                        add_scene_index(int32_t delta);
  [[nodiscard]] scene::Scene& current_scene();

  void add_workspace_manager(
      std::shared_ptr<proto::wayland::workspace::WorkspaceManager> manager);
  void remove_workspace_manager(wl_resource* manager);

  void for_each_scene_id(const std::function<void(int32_t, bool)>& callback) {
    for (int32_t i = 0; i < static_cast<int32_t>(scenes_.size()); ++i) {
      callback(i, i == current_scene_index_);
    }
  }

 private:
  // in [0, 9]
  int32_t                                current_scene_index_ = 1;
  std::array<scene::Scene, kSceneLength> scenes_;
  std::list<std::shared_ptr<proto::wayland::workspace::WorkspaceManager>>
      workspace_managers_;
};
}  // namespace yabe::core
