#pragma once

#include <wayland-server-core.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/surface.hpp"

namespace yabe::core::server_seat {
class ServerKeyboard {
 public:
  DISABLE_MOVE_AND_COPY(ServerKeyboard);
  explicit ServerKeyboard() = default;
  ~ServerKeyboard();

  void                   handle_keyboard_key(uint32_t key, bool is_pressed);
  /// @return a wl_array whose buffer is owned by std::vector,
  ///         so the caller must not release it
  [[nodiscard]] wl_array get_pressed_keys() const;

  void set_keymap(uint32_t format, int32_t fd, uint32_t size);
  void set_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
      uint32_t mods_locked, uint32_t group);
  void set_repeat_info(int32_t rate, int32_t delay);

  void send_init_events(wl_resource* resource);

  /// @return true if the focused surface is changed
  bool set_focused_surface(std::weak_ptr<Surface> surface);
  [[nodiscard]] const std::weak_ptr<Surface>& get_focused_surface() const {
    return this->focused_surface_;
  }

 private:
  std::vector<uint32_t> pressed_keys_;

  uint32_t keymap_format_ = 0;
  int32_t  keymap_fd_     = -1;
  uint32_t keymap_size_   = 0;

  uint32_t mods_depressed_ = 0;
  uint32_t mods_latched_   = 0;
  uint32_t mods_locked_    = 0;
  uint32_t mods_group_     = 0;

  int32_t repeat_rate_  = 33;
  int32_t repeat_delay_ = 500;

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
                         get_focused_client_seat() const;
  std::weak_ptr<Surface> focused_surface_;
};
}  // namespace yabe::core::server_seat
