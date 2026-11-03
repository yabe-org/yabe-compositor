#pragma once

#include <text-input-protocol.h>
#include <wayland-server.h>

#include <glm/ext.hpp>
#include <memory>
#include <optional>

#include "common/box.hpp"
#include "common/common.hpp"
#include "common/thread-safety.hpp"
#include "core/surface.hpp"
#include "protocols/wlr/input-method/input-method.hpp"
#include "protocols/wlr/input-method/input-popup.hpp"

namespace yabe::core::server_seat {
class ServerInputMethod {
 public:
  DISABLE_MOVE_AND_COPY(ServerInputMethod);
  ServerInputMethod()  = default;
  ~ServerInputMethod() = default;

  void draw(const glm::mat4& vp_mat);

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

  void set_input_popup(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method,
      std::shared_ptr<proto::wlr::input_method::InputPopup>&  popup);
  void try_clear_current_input_popup(
      std::shared_ptr<proto::wlr::input_method::InputMethod>& input_method);
  [[nodiscard]] bool has_input_popup() {
    const auto lock = this->mutex_.lock();
    return this->input_popup_.lock() != nullptr;
  }
  void renew_popup_parent(std::shared_ptr<core::Surface>& focused_surface);

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
  void set_cursor_rect(const common::Box2D& rect);

  void send_activate();
  void send_deactivate();
  void send_surrounding_text(
      const char* text, uint32_t cursor, uint32_t anchor);
  void send_text_change_cause(zwp_text_input_v3_change_cause cause);
  void send_content_type(
      uint32_t hint, zwp_text_input_v3_content_purpose purpose);
  void send_done();

  [[nodiscard]] std::optional<common::Box2D> current_cursor_rect() {
    const auto lock = this->mutex_.lock();
    return this->cursor_rect_;
  }

 private:
  struct {
    std::optional<std::string> commit_string;
    std::optional<std::string> preedit_string;
    std::optional<int32_t>     cursor_begin  = 0;
    std::optional<int32_t>     cursor_end    = 0;
    std::optional<uint32_t>    before_length = 0;
    std::optional<uint32_t>    after_length  = 0;
  } pending_, current_;
  std::optional<common::Box2D> cursor_rect_ GUARDED_BY(mutex_);
  common::Mutex                             mutex_;

  std::weak_ptr<Surface> parent_surface_               GUARDED_BY(mutex_);
  std::weak_ptr<proto::wlr::input_method::InputMethod> input_method_;
  std::weak_ptr<proto::wlr::input_method::InputPopup> input_popup_ GUARDED_BY(
      mutex_);
};
}  // namespace yabe::core::server_seat
