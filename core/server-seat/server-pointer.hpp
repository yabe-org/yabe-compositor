#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"
#include "core/scene/scene-node.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"

namespace yabe::core::server_seat {
class ServerPointer {
 public:
  DISABLE_MOVE_AND_COPY(ServerPointer);
  explicit ServerPointer();
  ~ServerPointer() = default;

  [[nodiscard]] const std::weak_ptr<scene::SceneNode>& get_focused_scene_node()
      const {
    return this->focused_scene_node_;
  }
  void unfocus_surface();
  void focus_pointed_surface();

  void               start_grab(pointer_grab::PointerGrab* grab);
  void               end_grab();
  [[nodiscard]] bool is_active_grab(pointer_grab::PointerGrab* grab) const {
    return this->grab_ == grab;
  }

  void handle_pointer_move_relative(float dx, float dy);
  void handle_pointer_button(uint32_t button, bool is_pressed);
  void handle_pointer_axis(uint32_t axis, float value);

 private:
  /// @return true if the focused surface node is changed
  bool set_focused_scene_node(std::shared_ptr<scene::SceneNode> scene_node);

  std::weak_ptr<scene::SceneNode> focused_scene_node_;
  pointer_grab::PointerGrab*      grab_;
};
}  // namespace yabe::core::server_seat
