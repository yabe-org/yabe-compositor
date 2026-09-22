#include "server-input-method.hpp"

#include <input-method-protocol.h>

#include <optional>

#include "core/server.hpp"

namespace yabe::core::server_seat {
void ServerInputMethod::set_input_method(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& resource) {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_unavailable(input_method->resource());
  }
  this->input_method_ = resource;
}
void ServerInputMethod::try_clear_current_input_method(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& resource) {
  if (is_current_input_method(resource)) {
    this->input_method_ = {};
  }
}

void ServerInputMethod::set_commit_string(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
    const char*                                             text) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.commit_string = text;
}
void ServerInputMethod::set_preedit_string(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
    const char* text, int32_t cursor_begin, int32_t cursor_end) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.preedit_string = text;
  this->pending_.cursor_begin   = cursor_begin;
  this->pending_.cursor_end     = cursor_end;
}
void ServerInputMethod::delete_surrounding_text(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
    uint32_t before_length, uint32_t after_length) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.before_length = before_length;
  this->pending_.after_length  = after_length;
}
void ServerInputMethod::commit(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->current_ = this->pending_;
  this->pending_ = {};
  auto keyboard_focused_surface =
      server::get().seat.keyboard.get_focused_surface().lock();
  if (!keyboard_focused_surface) {
    return;
  }
  auto client_seat =
      server::get().seat.get_client_seat(keyboard_focused_surface->client());
  if (!client_seat) {
    return;
  }

  if (this->current_.commit_string) {
    client_seat.value()->text_input.send_commit_string(
        this->current_.commit_string->c_str());
  }
  if (this->current_.preedit_string) {
    client_seat.value()->text_input.send_preedit_string(
        this->current_.preedit_string->c_str(), *this->current_.cursor_begin,
        *this->current_.cursor_end);
  }
  if (this->current_.before_length) {
    client_seat.value()->text_input.send_delete_surrounding_text(
        *this->current_.before_length, *this->current_.after_length);
  }
  client_seat.value()->text_input.send_done(input_method->serial());
}

void ServerInputMethod::send_activate() {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_activate(input_method->resource());
  }
}
void ServerInputMethod::send_deactivate() {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_deactivate(input_method->resource());
  }
}
void ServerInputMethod::send_surrounding_text(
    const char* text, uint32_t cursor, uint32_t anchor) {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_surrounding_text(
        input_method->resource(), text, cursor, anchor);
  }
}
void ServerInputMethod::send_text_change_cause(
    zwp_text_input_v3_change_cause cause) {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_text_change_cause(input_method->resource(), cause);
  }
}
void ServerInputMethod::send_content_type(
    uint32_t hint, zwp_text_input_v3_content_purpose purpose) {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_content_type(
        input_method->resource(), hint, purpose);
  }
}
void ServerInputMethod::send_done() {
  if (const auto input_method = this->input_method_.lock()) {
    zwp_input_method_v2_send_done(input_method->resource());
  }
}

bool ServerInputMethod::is_current_input_method(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& resource) {
  const auto input_method = this->input_method_.lock();
  return input_method == resource;
}
}  // namespace yabe::core::server_seat
