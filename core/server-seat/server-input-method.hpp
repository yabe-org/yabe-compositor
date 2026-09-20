#pragma once

#include <wayland-server.h>

#include <optional>

#include "common/common.hpp"
namespace yabe::core::server_seat {
class ServerInputMethod {
 public:
  DISABLE_MOVE_AND_COPY(ServerInputMethod);
  ServerInputMethod()  = default;
  ~ServerInputMethod() = default;

  /// if this class already have a input method,
  /// send unavailable() and overwrite it by given resource
  void set_input_method(wl_resource* resource);
  /// if given resource is set to this class,
  /// removve it without sending unavailable() event
  void try_clear_current_input_method(wl_resource* resource);

  void set_commit_string(wl_resource* input_method, const char* text);
  void set_preedit_string(wl_resource* input_method, const char* text,
      int32_t cursor_begin, int32_t cursor_end);
  void delete_surrounding_text(
      wl_resource* input_method, uint32_t before_length, uint32_t after_length);
  void commit(wl_resource* input_method, uint32_t serial);

 private:
  bool is_current_input_method(wl_resource* resource);

  struct {
    std::string commit_string;
    std::string preedit_string;
    int32_t     cursor_begin  = 0;
    int32_t     cursor_end    = 0;
    uint32_t    before_length = 0;
    uint32_t    after_length  = 0;
  } pending_, current_;

  std::optional<wl_resource*> input_method_;
};
}  // namespace yabe::core::server_seat
