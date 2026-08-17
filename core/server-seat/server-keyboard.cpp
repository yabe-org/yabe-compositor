#include "server-keyboard.hpp"

#include <linux/input-event-codes.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <optional>
#include <stdexcept>

#include "common/common.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
namespace {
void log_for_xkbcommon(struct xkb_context*, enum xkb_log_level level,
    const char* format, va_list args) {
  std::array<char, 1024> buffer{};
  (void)vsnprintf(buffer.data(), buffer.size(), format, args);
  const auto len = strlen(buffer.data());
  if (len > 0 && buffer.at(len - 1) == '\n') {
    buffer.at(len - 1) = '\0';
  }

  switch (level) {
    case XKB_LOG_LEVEL_CRITICAL:
    case XKB_LOG_LEVEL_ERROR:
      LOG_ERR("xkb: {}", buffer.data());
      break;
    case XKB_LOG_LEVEL_WARNING:
      LOG_WARN("xkb: {}", buffer.data());
      break;
    case XKB_LOG_LEVEL_INFO:
      LOG_INFO("xkb: {}", buffer.data());
      break;
    case XKB_LOG_LEVEL_DEBUG:
      LOG_DBG("xkb: {}", buffer.data());
      break;
    default:
      LOG_INFO("xkb: {}", buffer.data());
      break;
  }
}

int create_memfd(const char* name, off_t length) {
  auto fd = static_cast<int>(syscall(SYS_memfd_create, name, MFD_CLOEXEC));
  if (fd < 0) {
    return -1;
  }
  if (ftruncate(fd, length) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}
}  // namespace

ServerKeyboard::ServerKeyboard() {
  this->xkb_context_ = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
  if (!this->xkb_context_) {
    throw std::runtime_error("Failed to create xkb context");
  }
  xkb_context_set_log_level(this->xkb_context_, XKB_LOG_LEVEL_DEBUG);
  xkb_context_set_log_fn(this->xkb_context_, log_for_xkbcommon);
}
ServerKeyboard::~ServerKeyboard() {
  if (this->keymap_fd_ >= 0) {
    close(this->keymap_fd_);
  }
  if (this->xkb_keymap_) {
    xkb_keymap_unref(this->xkb_keymap_);
  }
  if (this->xkb_state_) {
    xkb_state_unref(this->xkb_state_);
  }
  xkb_context_unref(this->xkb_context_);
}

void ServerKeyboard::init_keymap(const char* rules, const char* model,
    const char* layout, const char* variant, const char* options) {
  const auto* keymap_str =
      this->create_keymap_(rules, model, layout, variant, options);
  if (!keymap_str) {
    LOG_ERR(
        "Failed to create xkb keymap for: rules={}, model={}, layout={}, "
        "variant={}, options={} (keymap string is nullptr)",
        rules, model, layout, variant, options);
    return;
  }
  auto* xkb_keymap = xkb_keymap_new_from_string(this->xkb_context_, keymap_str,
      XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!xkb_keymap) {
    LOG_ERR(
        "Failed to create xkb keymap for: rules={}, model={}, layout={}, "
        "variant={}, options={} (xkb_keymap_new_from_string() failed)",
        rules, model, layout, variant, options);
    return;
  }
  auto* xkb_state = xkb_state_new(xkb_keymap);
  if (!xkb_state) {
    LOG_ERR(
        "Failed to create xkb state for: rules={}, model={}, layout={}, "
        "variant={}, options={} (xkb_state_new() failed)",
        rules, model, layout, variant, options);
    xkb_keymap_unref(xkb_keymap);
    return;
  }
  const uint32_t size = strlen(keymap_str) + 1;  // + '\0'
  const auto     fd   = create_memfd("yabe-xkb-keymap", size);
  if (fd < 0) {
    LOG_ERR("Failed to create xkb keymap shared memory");
    xkb_keymap_unref(xkb_keymap);
    xkb_state_unref(xkb_state);
    return;
  }
  {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      LOG_ERR("Failed to map xkb keymap shared memory");
      xkb_keymap_unref(xkb_keymap);
      xkb_state_unref(xkb_state);
      close(fd);
      return;
    }
    std::memcpy(ptr, keymap_str, size);
    munmap(ptr, size);
  }

  if (this->xkb_keymap_) {
    xkb_keymap_unref(this->xkb_keymap_);
  }
  if (this->xkb_state_) {
    xkb_state_unref(this->xkb_state_);
  }
  if (this->keymap_fd_ >= 0) {
    close(this->keymap_fd_);
  }
  this->xkb_keymap_  = xkb_keymap;
  this->xkb_state_   = xkb_state;
  this->keymap_fd_   = fd;
  this->keymap_size_ = size;

  server::get().seat.foreach ([this](client_seat::ClientSeat& client_seat) {
    client_seat.keyboard.send_keymap(
        XKB_KEYMAP_FORMAT_TEXT_V1, this->keymap_fd_, this->keymap_size_);
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

void ServerKeyboard::handle_keyboard_key(uint32_t key, bool is_pressed) {
  if (this->xkb_state_ == nullptr) {
    return;
  }
  xkb_state_update_key(
      this->xkb_state_, key + 8, is_pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
  this->update_modifiers();

  const auto it =
      std::find(this->pressed_keys_.begin(), this->pressed_keys_.end(), key);
  if (is_pressed) {
    if (it == this->pressed_keys_.end()) {
      this->pressed_keys_.push_back(key);
    }
  } else if (it != this->pressed_keys_.end()) {
    this->pressed_keys_.erase(it);
  }

  const auto& client_seat = this->get_focused_client_seat();
  if (client_seat) {
    client_seat.value()->keyboard.send_key(key, is_pressed);
  }
}
void ServerKeyboard::update_modifiers() {
  const auto depressed =
      xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_DEPRESSED);
  const auto latched =
      xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_LATCHED);
  const auto locked =
      xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_LOCKED);
  const auto group =
      xkb_state_serialize_layout(this->xkb_state_, XKB_STATE_LAYOUT_EFFECTIVE);

  if (this->mods_depressed_ == depressed && this->mods_latched_ == latched &&
      this->mods_locked_ == locked && this->mods_group_ == group) {
    return;
  }

  this->mods_depressed_ = depressed;
  this->mods_latched_   = latched;
  this->mods_locked_    = locked;
  this->mods_group_     = group;
  server::get().seat.foreach ([this](client_seat::ClientSeat& client_seat) {
    client_seat.keyboard.send_modifiers(this->mods_depressed_,
        this->mods_latched_, this->mods_locked_, this->mods_group_);
  });
}

void ServerKeyboard::send_init_events(wl_resource* resource) {
  if (this->keymap_fd_ >= 0) {
    wl_keyboard_send_keymap(resource, XKB_KEYMAP_FORMAT_TEXT_V1,
        this->keymap_fd_, this->keymap_size_);
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

bool ServerKeyboard::is_key_pressed(xkb_keysym_t keysym) const {
  if (!this->xkb_state_) {
    return false;
  }
  return std::ranges::any_of(this->pressed_keys_,  //
      [this, keysym](uint32_t keycode) {
        auto current_sym =
            xkb_state_key_get_one_sym(this->xkb_state_, keycode + 8);
        // convert upper because, e.g. in XKB_KEY_A,
        // 'A' is upper char and feel more like constant
        return xkb_keysym_to_upper(current_sym) == keysym;
      });
}
bool ServerKeyboard::is_mod_pressed(ModKeys mod) const {
  if (!this->xkb_state_) {
    return false;
  }
  const char* name = nullptr;
  switch (mod) {
    case ModKeys::CTRL:
      name = XKB_MOD_NAME_CTRL;
      break;
    case ModKeys::ALT:
      name = XKB_MOD_NAME_ALT;
      break;
    case ModKeys::SHIFT:
      name = XKB_MOD_NAME_SHIFT;
      break;
    case ModKeys::SUPER:
      name = XKB_MOD_NAME_LOGO;
      break;
    case ModKeys::CAPS:
      name = XKB_MOD_NAME_CAPS;
      break;
    case ModKeys::NUM:
      name = XKB_MOD_NAME_MOD2;
      break;
    case ModKeys::MOD3:
      name = XKB_MOD_NAME_MOD3;
      break;
    case ModKeys::MOD5:
      name = XKB_MOD_NAME_MOD5;
      break;
  }
  return xkb_state_mod_name_is_active(
             this->xkb_state_, name, XKB_STATE_MODS_EFFECTIVE) == 1;
}

wl_array ServerKeyboard::get_pressed_keys() const {
  return wl_array{
      .size  = this->pressed_keys_.size() * sizeof(uint32_t),
      .alloc = 0,
      .data  = const_cast<uint32_t*>(this->pressed_keys_.data()),  // NOLINT
  };
}
}  // namespace yabe::core::server_seat
