#include "client-text-input.hpp"

#include <text-input-protocol.h>

#include "core/server.hpp"
#include "protocols/wayland/text-input/text-input.hpp"

namespace yabe::core::client_seat {
void ClientTextInput::add_text_input(wl_resource* resource) {
  LOG_DBG("add text_input#{}", wl_resource_get_id(resource));
  this->text_inputs_.push_back(resource);
}
void ClientTextInput::remove_text_input(wl_resource* resource) {
  this->text_inputs_.remove(resource);
}

void ClientTextInput::send_enter(const std::shared_ptr<Surface>& surface) {
  for (auto* resource : this->text_inputs_) {
    zwp_text_input_v3_send_enter(resource, surface->resource());
  }
}
void ClientTextInput::send_leave(const std::shared_ptr<Surface>& surface) {
  for (auto* resource : this->text_inputs_) {
    zwp_text_input_v3_send_leave(resource, surface->resource());
  }
}

void ClientTextInput::send_preedit_string(
    const char* text, int32_t cursor_begin, int32_t cursor_end) {
  for (auto* resource : this->text_inputs_) {
    zwp_text_input_v3_send_preedit_string(
        resource, text, cursor_begin, cursor_end);
  }
}
void ClientTextInput::send_commit_string(const char* text) {
  for (auto* resource : this->text_inputs_) {
    zwp_text_input_v3_send_commit_string(resource, text);
  }
}
void ClientTextInput::send_delete_surrounding_text(
    uint32_t before_length, uint32_t after_length) {
  for (auto* resource : this->text_inputs_) {
    zwp_text_input_v3_send_delete_surrounding_text(
        resource, before_length, after_length);
  }
}
void ClientTextInput::send_done() {
  for (auto* resource : this->text_inputs_) {
    const auto text_input =
        server::get().resources.get<proto::wayland::text_input::TextInput>(
            resource);
    if (text_input) {
      zwp_text_input_v3_send_done(resource, text_input->serial());
    }
  }
}
void ClientTextInput::send_action(uint32_t action, uint32_t serial) {
  for (auto* resource : this->text_inputs_) {
    if (wl_resource_get_version(resource) <
        ZWP_TEXT_INPUT_V3_ACTION_SINCE_VERSION) {
      continue;
    }
    zwp_text_input_v3_send_action(resource, action, serial);
  }
}
void ClientTextInput::send_language(const char* language) {
  for (auto* resource : this->text_inputs_) {
    if (wl_resource_get_version(resource) <
        ZWP_TEXT_INPUT_V3_LANGUAGE_SINCE_VERSION) {
      continue;
    }
    zwp_text_input_v3_send_language(resource, language);
  }
}
void ClientTextInput::send_preedit_hint(
    uint32_t start, uint32_t end, uint32_t hint) {
  for (auto* resource : this->text_inputs_) {
    if (wl_resource_get_version(resource) <
        ZWP_TEXT_INPUT_V3_PREEDIT_HINT_SINCE_VERSION) {
      continue;
    }
    zwp_text_input_v3_send_preedit_hint(resource, start, end, hint);
  }
}
}  // namespace yabe::core::client_seat
