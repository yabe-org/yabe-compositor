#include "server-keyboard.hpp"

#include <unistd.h>

#include <optional>

#include "core/server.hpp"

namespace yabe::core::server_seat {
void ServerKeyboard::handle_keyboard_key(uint32_t key, bool is_pressed) {
  if (is_pressed) {
    if (std::find(this->pressed_keys_.begin(), this->pressed_keys_.end(),
            key) == this->pressed_keys_.end()) {
      this->pressed_keys_.push_back(key);
    }
  } else {
    this->pressed_keys_.erase(std::remove(this->pressed_keys_.begin(),
                                  this->pressed_keys_.end(), key),
        this->pressed_keys_.end());
  }
  const auto& client_seat = this->get_focused_client_seat();
  if (client_seat) {
    client_seat.value()->keyboard.send_key(key, is_pressed);
  }
}

wl_array ServerKeyboard::get_pressed_keys() const {
  return wl_array{
      .size  = this->pressed_keys_.size() * sizeof(uint32_t),
      .alloc = 0,
      .data  = const_cast<uint32_t*>(this->pressed_keys_.data()),  // NOLINT
  };
}

ServerKeyboard::~ServerKeyboard() {
  if (this->keymap_fd_ >= 0) {
    close(this->keymap_fd_);
  }
}

void ServerKeyboard::set_keymap(uint32_t format, int32_t fd, uint32_t size) {
  if (this->keymap_fd_ >= 0) {
    close(this->keymap_fd_);
  }
  this->keymap_format_ = format;
  this->keymap_fd_     = fd >= 0 ? dup(fd) : -1;
  this->keymap_size_   = size;

  if (this->keymap_fd_ >= 0) {
    server::get().seat.foreach ([this](client_seat::ClientSeat& client_seat) {
      client_seat.keyboard.send_keymap(
          this->keymap_format_, this->keymap_fd_, this->keymap_size_);
    });
  }
}

void ServerKeyboard::set_modifiers(uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
  this->mods_depressed_ = mods_depressed;
  this->mods_latched_   = mods_latched;
  this->mods_locked_    = mods_locked;
  this->mods_group_     = group;

  server::get().seat.foreach ([this](client_seat::ClientSeat& client_seat) {
    client_seat.keyboard.send_modifiers(this->mods_depressed_,
        this->mods_latched_, this->mods_locked_, this->mods_group_);
  });
}

void ServerKeyboard::set_repeat_info(int32_t rate, int32_t delay) {
  this->repeat_rate_  = rate;
  this->repeat_delay_ = delay;

  server::get().seat.foreach ([this](client_seat::ClientSeat& client_seat) {
    client_seat.keyboard.send_repeat_info(
        this->repeat_rate_, this->repeat_delay_);
  });
}

void ServerKeyboard::send_init_events(wl_resource* resource) {
  if (this->keymap_fd_ >= 0) {
    wl_keyboard_send_keymap(
        resource, this->keymap_format_, this->keymap_fd_, this->keymap_size_);
  }
  if (wl_resource_get_version(resource) >=
      WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
    wl_keyboard_send_repeat_info(
        resource, this->repeat_rate_, this->repeat_delay_);
  }

  auto focused_surface = this->focused_surface_.lock();
  if (focused_surface &&
      wl_resource_get_client(resource) == focused_surface->client()) {
    auto       keys   = this->get_pressed_keys();
    const auto serial = server::get().next_serial();
    wl_keyboard_send_enter(
        resource, serial, focused_surface->resource(), &keys);
    wl_keyboard_send_modifiers(resource, serial, this->mods_depressed_,
        this->mods_latched_, this->mods_locked_, this->mods_group_);
  }
}

std::optional<std::shared_ptr<client_seat::ClientSeat>>
ServerKeyboard::get_focused_client_seat() const {
  auto focused_surface = this->focused_surface_.lock();
  if (!focused_surface) {
    return std::nullopt;
  }
  return server::get().seat.get_client_seat(focused_surface->client());
}

bool ServerKeyboard::set_focused_surface(std::weak_ptr<Surface> surface) {
  if (this->focused_surface_.lock() == surface.lock()) {
    return false;
  }
  if (auto focused_surface = this->focused_surface_.lock()) {
    const auto& client_seat =
        server::get().seat.get_client_seat(focused_surface->client());
    if (client_seat) {
      client_seat.value()->keyboard.send_leave(focused_surface->resource());
    }
  }
  this->focused_surface_ = surface;
  if (auto focused_surface = this->focused_surface_.lock()) {
    const auto& client_seat =
        server::get().seat.get_client_seat(focused_surface->client());
    if (client_seat) {
      auto pressed_keys = this->get_pressed_keys();
      client_seat.value()->keyboard.send_enter(
          focused_surface->resource(), &pressed_keys);
      client_seat.value()->keyboard.send_modifiers(this->mods_depressed_,
          this->mods_latched_, this->mods_locked_, this->mods_group_);
    }
  }
  return true;
}
}  // namespace yabe::core::server_seat
