#pragma once

#include <wayland-server.h>

#include <cstdint>
#include <list>
#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"

namespace yabe::core::client_seat {
class ClientTextInput {
 public:
  DISABLE_MOVE_AND_COPY(ClientTextInput);
  explicit ClientTextInput() = default;
  ~ClientTextInput()         = default;

  void add_text_input(wl_resource* resource);
  void remove_text_input(wl_resource* resource);

  void send_enter(const std::shared_ptr<Surface>& surface);
  void send_leave(const std::shared_ptr<Surface>& surface);
  void send_preedit_string(
      const char* text, int32_t cursor_begin, int32_t cursor_end);
  void send_commit_string(const char* text);
  void send_delete_surrounding_text(
      uint32_t before_length, uint32_t after_length);
  void send_done(uint32_t serial);
  void send_action(uint32_t action, uint32_t serial);
  void send_language(const char* language);
  void send_preedit_hint(uint32_t start, uint32_t end, uint32_t hint);

 private:
  std::list<wl_resource*> text_inputs_;
};
}  // namespace yabe::core::client_seat
