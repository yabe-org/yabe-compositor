#pragma once

#include <memory>

#include "common/common.hpp"
#include "core/scene/scene-node.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"

namespace yabe::core::server_seat::pointer_grab {
class MovePointerGrab final : public PointerGrab {
 public:
  DISABLE_MOVE_AND_COPY(MovePointerGrab);

  static MovePointerGrab* start(std::weak_ptr<scene::SceneNode>&& scene_node);

  void cleanup() override;
  void move_relative(float dx, float dy) override;
  void button(uint32_t button, bool is_pressed) override;

 private:
  explicit MovePointerGrab() = default;
  ~MovePointerGrab()         = default;

  std::weak_ptr<scene::SceneNode> moving_scene_node_;
};
}  // namespace yabe::core::server_seat::pointer_grab
