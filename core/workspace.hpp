#pragma once

#include "common/common.hpp"
#include "core/scene/scene.hpp"

namespace yabe::core {
class Workspace {
 public:
  DISABLE_MOVE_AND_COPY(Workspace);
  Workspace()  = default;
  ~Workspace() = default;

  // @param index must be [0, 9]
  void                        set_current_scene_index(int32_t index);
  void                        add_scene_index(int32_t delta);
  [[nodiscard]] scene::Scene& current_scene();

  void for_each_scene_id(const std::function<void(int32_t, bool)>& callback) {
    for (int32_t i = 0; i < static_cast<int32_t>(scenes_.size()); ++i) {
      callback(i, i == current_scene_index_);
    }
  }

 private:
  // in [0, 9]
  int32_t                      current_scene_index_ = 1;
  std::array<scene::Scene, 10> scenes_;
};
}  // namespace yabe::core
