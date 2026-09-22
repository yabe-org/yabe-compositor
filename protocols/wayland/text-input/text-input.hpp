#pragma once

#include <text-input-protocol.h>
#include <wayland-server-core.h>

#include <cstdint>

#include "common/box.hpp"
#include "common/common.hpp"

namespace yabe::proto::wayland::text_input {
class TextInput {
 public:
  DISABLE_MOVE_AND_COPY(TextInput);
  TextInput()  = default;
  ~TextInput() = default;

  void set_enabled(bool is_enabled);
  void set_surrounding_text(const char* text, int32_t cursor, int32_t anchor);
  void set_text_change_cause(zwp_text_input_v3_change_cause cause);
  void set_content_type(
      uint32_t hint, zwp_text_input_v3_content_purpose purpose);
  void set_cursor_rectangle(
      int32_t x, int32_t y, int32_t width, int32_t height);
  void commit();

 private:
  struct {
    std::optional<bool>                              is_enabled;
    std::optional<std::string>                       text;
    std::optional<int32_t>                           cursor;
    std::optional<int32_t>                           anchor;
    std::optional<zwp_text_input_v3_change_cause>    cause;
    std::optional<uint32_t>                          hint;
    std::optional<zwp_text_input_v3_content_purpose> purpose;
    std::optional<common::Box2D>                     cursor_rect;
  } pending_, current_;
};

void create_text_input(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::wayland::text_input
