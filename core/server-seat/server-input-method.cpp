#include "server-input-method.hpp"

#include <input-method-protocol.h>

#include <optional>

#include "core/server.hpp"

namespace yabe::core::server_seat {
void ServerInputMethod::set_input_method(wl_resource* resource) {
  if (this->input_method_) {
    zwp_input_method_v2_send_unavailable(*this->input_method_);
  }
  this->input_method_ = resource;
}
void ServerInputMethod::try_clear_current_input_method(wl_resource* resource) {
  if (is_current_input_method(resource)) {
    this->input_method_ = std::nullopt;
  }
}

void ServerInputMethod::set_commit_string(
    wl_resource* input_method, const char* text) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.commit_string = text;
}
void ServerInputMethod::set_preedit_string(wl_resource* input_method,
    const char* text, int32_t cursor_begin, int32_t cursor_end) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.preedit_string = text;
  this->pending_.cursor_begin   = cursor_begin;
  this->pending_.cursor_end     = cursor_end;
}
void ServerInputMethod::delete_surrounding_text(
    wl_resource* input_method, uint32_t before_length, uint32_t after_length) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  this->pending_.before_length = before_length;
  this->pending_.after_length  = after_length;
}
void ServerInputMethod::commit(wl_resource* input_method, uint32_t serial) {
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
  client_seat.value()->text_input.send_commit_string(
      this->current_.commit_string.c_str());
  client_seat.value()->text_input.send_preedit_string(
      this->current_.preedit_string.c_str(), this->current_.cursor_begin,
      this->current_.cursor_end);
  client_seat.value()->text_input.send_delete_surrounding_text(
      this->current_.before_length, this->current_.after_length);
  client_seat.value()->text_input.send_done(serial);
}

bool ServerInputMethod::is_current_input_method(wl_resource* resource) {
  if (!this->input_method_) {
    return false;
  }
  if (*this->input_method_ != resource) {
    return false;
  }
  return true;
}
}  // namespace yabe::core::server_seat
