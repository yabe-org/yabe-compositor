#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server-seat/pointer-grab/pointer-grab.hpp"

namespace yabe::core::server_seat {
class ServerPointer {
 public:
  DISABLE_MOVE_AND_COPY(ServerPointer);
  explicit ServerPointer();
  ~ServerPointer() = default;

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
  get_focused_client_seat() const;
  [[nodiscard]] const std::weak_ptr<scene::SurfaceNode>&
  get_focused_surface_node() const {
    return this->focused_surface_node_;
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
  bool set_focused_surface_node(std::weak_ptr<scene::SurfaceNode> surface_node);

  std::weak_ptr<scene::SurfaceNode> focused_surface_node_;
  pointer_grab::PointerGrab*        grab_;
};
}  // namespace yabe::core::server_seat
