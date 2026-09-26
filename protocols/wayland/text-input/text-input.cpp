#include "text-input.hpp"

#include <text-input-protocol.h>
#include <wayland-server-core.h>

#include "common/common.hpp"
#include "core/server.hpp"

namespace yabe::proto::wayland::text_input {
void TextInput::set_enabled(bool is_enabled) {
  // zwp_text_input_v3.enable() resets previously set values
  this->pending_            = {};
  this->pending_.is_enabled = is_enabled;
}

void TextInput::set_surrounding_text(
    const char* text, int32_t cursor, int32_t anchor) {
  this->pending_.text   = text;
  this->pending_.cursor = cursor;
  this->pending_.anchor = anchor;
}

void TextInput::set_text_change_cause(zwp_text_input_v3_change_cause cause) {
  this->pending_.cause = cause;
}

void TextInput::set_content_type(
    uint32_t hint, zwp_text_input_v3_content_purpose purpose) {
  this->pending_.hint    = hint;
  this->pending_.purpose = purpose;
}

void TextInput::set_cursor_rectangle(
    int32_t x, int32_t y, int32_t width, int32_t height) {
  this->pending_.cursor_rect = common::Box2D{
      .x{x},
      .y{y},
      .width{width},
      .height{height},
  };
}

void TextInput::commit() {
  ++this->serial_;
  auto& input_method = core::server::get().seat.input_method;
  if (this->pending_.is_enabled.has_value()) {
    this->current_.is_enabled = this->pending_.is_enabled;
    if (*this->current_.is_enabled) {
      input_method.send_activate();
    } else {
      input_method.send_deactivate();
    }
  }
  if (this->pending_.cursor_rect) {
    this->current_.cursor_rect = this->pending_.cursor_rect;
    if (this->current_.is_enabled.value_or(false)) {
      input_method.set_cursor_rect(*this->current_.cursor_rect);
    }
  }
  if (this->pending_.text) {
    this->current_.text   = this->pending_.text;
    this->current_.cursor = this->pending_.cursor;
    this->current_.anchor = this->pending_.anchor;
    input_method.send_surrounding_text(this->current_.text->c_str(),
        *this->current_.cursor, *this->current_.anchor);
  }
  if (this->pending_.cause) {
    this->current_.cause = this->pending_.cause;
    input_method.send_text_change_cause(*this->current_.cause);
  }
  if (this->pending_.hint) {
    this->current_.hint    = this->pending_.hint;
    this->current_.purpose = this->pending_.purpose;
    input_method.send_content_type(
        *this->current_.hint, *this->current_.purpose);
  }
  input_method.send_done();
  this->pending_ = {};
}

namespace {
constexpr struct zwp_text_input_v3_interface kImpl{
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .enable =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_enabled(true);
          }
        },
    .disable =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_enabled(false);
          }
        },
    .set_surrounding_text =
        [](wl_client* /*client*/, wl_resource* resource, const char* text,
            int32_t cursor, int32_t anchor) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_surrounding_text(text, cursor, anchor);
          }
        },
    .set_text_change_cause =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t cause) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_text_change_cause(
                static_cast<zwp_text_input_v3_change_cause>(cause));
          }
        },
    .set_content_type =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t hint,
            uint32_t purpose) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_content_type(
                hint, static_cast<zwp_text_input_v3_content_purpose>(purpose));
          }
        },
    .set_cursor_rectangle =
        [](wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y,
            int32_t width, int32_t height) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->set_cursor_rectangle(x, y, width, height);
          }
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<TextInput>(resource);
          if (self) {
            self->commit();
          }
        },
    .set_available_actions =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            wl_array* /*available_actions*/) {
          // todo?
        },
    .show_input_panel =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // todo?
        },
    .hide_input_panel =
        [](wl_client* /*client*/, wl_resource* /*resource*/) {
          // todo?
        },
};
}  // namespace

void create_text_input(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwp_text_input_v3_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);

  const auto client_seat = core::server::get().seat.get_client_seat(client);
  if (!client_seat) {
    LOG_ERR("client_seat not found on wl_keyboard.create");
    wl_resource_destroy(resource);
    return;
  }
  client_seat.value()->text_input.add_text_input(resource);

  auto self = std::make_shared<TextInput>();
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::wayland::text_input
