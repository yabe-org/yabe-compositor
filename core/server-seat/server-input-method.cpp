#include "server-input-method.hpp"

#include <input-method-protocol.h>

#include <glm/ext/matrix_transform.hpp>
#include <optional>

#include "common/box.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
void ServerInputMethod::draw(const glm::mat4& vp_mat) {
  common::Box2D                                         cursor_rect;
  std::shared_ptr<core::Surface>                        parent_surface;
  std::shared_ptr<proto::wlr::input_method::InputPopup> popup;
  {
    const auto lock = this->mutex_.lock();
    if (!this->cursor_rect_) {
      return;
    }
    cursor_rect    = *this->cursor_rect_;
    parent_surface = this->parent_surface_.lock();
    popup          = this->input_popup_.lock();
  }
  if (!parent_surface) {
    return;
  }
  const auto parent_node = parent_surface->surface_node();
  if (!parent_node) {
    return;
  }
  if (!popup) {
    return;
  }
  const auto popup_surface = popup->surface().lock();
  if (!popup_surface) {
    return;
  }
  popup_surface->upload_texture_if_needed();

  glm::mat4 parent_world_transform;
  {
    const auto lock        = scene::Scene::mutex.lock();
    parent_world_transform = parent_node.value()->world_transform();
  }
  const auto popup_width  = *popup_surface->texture_width().to_world_unit();
  const auto popup_height = *popup_surface->texture_height().to_world_unit();

  const glm::vec3 popup_center{
      +*cursor_rect.x.to_world_unit() + (popup_width / 2.F),
      -*(cursor_rect.y + cursor_rect.height).to_world_unit() -
          (popup_height / 2.F),
      0.F,
  };
  // actual position is based on left-top position of the parent
  const glm::vec3 pos{
      popup_center.x - (parent_node.value()->geometry().scale().x / 2.F),
      popup_center.y + (parent_node.value()->geometry().scale().y / 2.F),
      0.F,
  };
  const auto model_mat =
      vp_mat * parent_world_transform * glm::translate(glm::mat4(1.F), pos) *
      glm::scale(glm::mat4(1.F), {popup_width, popup_height, 1.F});
  server::get().rect_renderer.render(model_mat, popup_surface->texture());
}

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

void ServerInputMethod::set_input_popup(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
    std::shared_ptr<proto::wlr::input_method::InputPopup>&  popup) {
  if (!is_current_input_method(input_method)) {
    return;
  }
  {
    const auto lock    = this->mutex_.lock();
    this->input_popup_ = popup;
    if (this->cursor_rect_) {
      popup->send_text_input_rectangle(*this->cursor_rect_);
    }
  }
  auto focused_surface =
      server::get().seat.keyboard.get_focused_surface().lock();
  if (focused_surface) {
    this->renew_popup_parent(focused_surface);
  }
}
void ServerInputMethod::try_clear_current_input_popup(
    std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method) {
  if (is_current_input_method(input_method)) {
    const auto lock    = this->mutex_.lock();
    this->input_popup_ = {};
  }
}
void ServerInputMethod::renew_popup_parent(
    std::shared_ptr<core::Surface>& focused_surface) {
  if (!focused_surface || !focused_surface->surface_node()) {
    return;
  }
  const auto lock       = this->mutex_.lock();
  this->parent_surface_ = focused_surface;
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
  client_seat.value()->text_input.send_done();
}
void ServerInputMethod::set_cursor_rect(const common::Box2D& rect) {
  std::shared_ptr<proto::wlr::input_method::InputPopup> popup;
  {
    const auto lock    = this->mutex_.lock();
    this->cursor_rect_ = rect;
    popup              = this->input_popup_.lock();
  }
  if (popup) {
    popup->send_text_input_rectangle(rect);
  }
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
