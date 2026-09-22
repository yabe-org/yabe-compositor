#pragma once

#include <text-input-protocol.h>
#include <wayland-server.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "protocols/wlr/input-method/input-method.hpp"

namespace yabe::core::server_seat {
class ServerInputMethod {
 public:
  DISABLE_MOVE_AND_COPY(ServerInputMethod);
  ServerInputMethod()  = default;
  ~ServerInputMethod() = default;

  /// if this class already have a input method,
  /// send unavailable() and overwrite it by given resource
  void set_input_method(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& resource);
  bool is_current_input_method(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& resource);
  /// if given resource is set to this class,
  /// removve it without sending unavailable() event
  void try_clear_current_input_method(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& resource);

  void set_commit_string(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
      const char*                                             text);
  void set_preedit_string(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
      const char* text, int32_t cursor_begin, int32_t cursor_end);
  void delete_surrounding_text(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
      uint32_t before_length, uint32_t after_length);
  void commit(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method);

  void send_activate();
  void send_deactivate();
  void send_surrounding_text(
      const char* text, uint32_t cursor, uint32_t anchor);
  void send_text_change_cause(zwp_text_input_v3_change_cause cause);
  void send_content_type(
      uint32_t hint, zwp_text_input_v3_content_purpose purpose);
  void send_done();

 private:
  struct {
    std::optional<std::string> commit_string;
    std::optional<std::string> preedit_string;
    std::optional<int32_t>     cursor_begin  = 0;
    std::optional<int32_t>     cursor_end    = 0;
    std::optional<uint32_t>    before_length = 0;
    std::optional<uint32_t>    after_length  = 0;
  } pending_, current_;

  std::weak_ptr<proto::wlr::input_method::InputMethod> input_method_;
};
}  // namespace yabe::core::server_seat
