#include "server-keyboard.hpp"

#include <linux/input-event-codes.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include <memory>
#include <optional>

#include "common/common.hpp"
#include "common/time.hpp"
#include "core/keyboard.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
namespace {
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

ServerKeyboard::ServerKeyboard()
    : external_keyboard_(std::make_shared<Keyboard>()) {
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
  const uint32_t size = strlen(keymap_str) + 1;  // + '\0'
  const auto     fd   = create_memfd("yabe-xkb-keymap", size);
  if (fd < 0) {
    LOG_ERR("Failed to create xkb keymap shared memory");
    return;
  }
  {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      LOG_ERR("Failed to map xkb keymap shared memory");
      close(fd);
      return;
    }
    std::memcpy(ptr, keymap_str, size);
    munmap(ptr, size);
  }
  this->external_keyboard_->set_keymap(
      WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, size);
  close(fd);
}
void ServerKeyboard::initialize_wl_keyboard(wl_resource* wl_keyboard) {
  auto*      client      = wl_resource_get_client(wl_keyboard);
  const auto client_seat = server::get().seat.get_client_seat(client);
  if (!client_seat) {
    return;
  }
  // FIXME: using wl_keyboard_send_* should be called only from ClientKeyboard
  const auto focused_surface = this->focused_surface_.lock();
  if (focused_surface && focused_surface->client() == client) {
    auto pressed_keys = this->current_keyboard()->get_pressed_keys();
    wl_keyboard_send_enter(wl_keyboard, server::get().next_serial(),
        focused_surface->resource(), &pressed_keys);
  }
  wl_keyboard_send_keymap(wl_keyboard,
      this->current_keyboard()->keymap_format(),
      this->current_keyboard()->keymap_fd(),
      this->current_keyboard()->keymap_size());
  wl_keyboard_send_modifiers(wl_keyboard, server::get().next_serial(),
      this->current_keyboard()->mods().depressed,
      this->current_keyboard()->mods().latched,
      this->current_keyboard()->mods().locked,
      this->current_keyboard()->mods().group);
  if (wl_resource_get_version(wl_keyboard) >=
      WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
    wl_keyboard_send_repeat_info(wl_keyboard,
        this->current_keyboard()->repeat_rate(),
        this->current_keyboard()->repeat_delay());
  }
}

void ServerKeyboard::handle_external_keyboard_key(
    uint32_t key, bool is_pressed) {
  if (!this->external_keyboard_->is_valid()) {
    return;
  }
  const auto mods_updated =
      this->external_keyboard_->update_xkb_state(key, is_pressed);
  this->external_keyboard_->handle_key(key, is_pressed);
  const auto keysym = this->external_keyboard_->get_one_sym(key + 8);
  if (!server::get().setting.keybinds.handle_key(
          keysym, is_pressed, this->external_keyboard_->current_mod_mask())) {
    this->send_key(
        this->external_keyboard_, common::now_msec(), key, is_pressed);
  }
  if (mods_updated) {
    this->send_modifiers(this->external_keyboard_,
        this->external_keyboard_->mods().depressed,
        this->external_keyboard_->mods().latched,
        this->external_keyboard_->mods().locked,
        this->external_keyboard_->mods().group);
  }
}
void ServerKeyboard::send_key(std::shared_ptr<Keyboard> keyboard, uint32_t time,
    uint32_t key, bool is_pressed) {
  this->set_current_keyboard(keyboard);
  if (const auto grab = this->keyboard_grab_.lock()) {
    const auto keyboard_owner = keyboard->owner();
    if (!keyboard_owner || keyboard_owner != grab->owner()) {
      grab->send_key(server::get().next_serial(), time, key, is_pressed);
      return;
    }
  }
  if (const auto client_seat = get_focused_client_seat()) {
    client_seat.value()->keyboard.send_key(time, key, is_pressed);
  }
}
void ServerKeyboard::send_modifiers(std::shared_ptr<Keyboard> keyboard,
    uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group) {
  this->set_current_keyboard(keyboard);
  if (const auto grab = this->keyboard_grab_.lock()) {
    const auto keyboard_owner = keyboard->owner();
    if (!keyboard_owner || keyboard_owner != grab->owner()) {
      grab->send_modifiers(mods_depressed, mods_latched, mods_locked, group);
      return;
    }
  }
  if (const auto client_seat = get_focused_client_seat()) {
    client_seat.value()->keyboard.send_modifiers(
        mods_depressed, mods_latched, mods_locked, group);
  }
}
void ServerKeyboard::handle_keyboard_keymap_changed(
    std::shared_ptr<Keyboard> keyboard) {
  if (this->current_keyboard_.lock() != keyboard) {
    return;
  }
  if (const auto grab = this->keyboard_grab_.lock()) {
    if (grab->is_current_keyboard(keyboard)) {
      grab->send_keymap(keyboard->keymap_format(), keyboard->keymap_fd(),
          keyboard->keymap_size());
    }
  }
  const auto client_seat = this->get_focused_client_seat();
  server::get().seat.foreach (
      [keyboard](std::shared_ptr<client_seat::ClientSeat>& client_seat) {
        if (client_seat->keyboard.is_current_keyboard(keyboard)) {
          client_seat->keyboard.send_keymap(keyboard->keymap_format(),
              keyboard->keymap_fd(), keyboard->keymap_size());
        }
      });
}
void ServerKeyboard::handle_keyboard_repeat_info_changed(
    std::shared_ptr<Keyboard> keyboard) {
  if (this->current_keyboard_.lock() != keyboard) {
    return;
  }
  if (const auto grab = this->keyboard_grab_.lock()) {
    if (grab->is_current_keyboard(keyboard)) {
      grab->send_repeat_info(keyboard->repeat_rate(), keyboard->repeat_delay());
    }
  }
  server::get().seat.foreach (
      [keyboard](std::shared_ptr<client_seat::ClientSeat>& client_seat) {
        if (client_seat->keyboard.is_current_keyboard(keyboard)) {
          client_seat->keyboard.send_repeat_info(
              keyboard->repeat_rate(), keyboard->repeat_delay());
        }
      });
}
void ServerKeyboard::set_current_keyboard(std::shared_ptr<Keyboard>& keyboard) {
  if (this->current_keyboard_.lock() != keyboard) {
    this->current_keyboard_ = keyboard;
  }
  if (const auto grab = this->keyboard_grab_.lock()) {
    if (!grab->is_current_keyboard(keyboard)) {
      grab->set_current_keyboard(keyboard);
    }
    return;
  }
  const auto client_seat = get_focused_client_seat();
  if (client_seat &&
      !client_seat.value()->keyboard.is_current_keyboard(keyboard)) {
    client_seat.value()->keyboard.set_current_keyboard(keyboard);
  }
}
void ServerKeyboard::send_initial_events(
    const std::shared_ptr<client_seat::ClientSeat>& client_seat) {
  auto keyboard = this->current_keyboard();
  client_seat->keyboard.send_keymap(keyboard->keymap_format(),
      keyboard->keymap_fd(), keyboard->keymap_size());
  client_seat->keyboard.send_repeat_info(
      keyboard->repeat_rate(), keyboard->repeat_delay());
  client_seat->keyboard.send_modifiers(keyboard->mods().depressed,
      keyboard->mods().latched, keyboard->mods().locked,
      keyboard->mods().group);
}

void ServerKeyboard::set_keyboard_grab(
    std::shared_ptr<proto::wlr::input_method::KeyboardGrab>& grab) {
  this->keyboard_grab_ = grab;
  grab->send_keymap(this->current_keyboard()->keymap_format(),
      this->current_keyboard()->keymap_fd(),
      this->current_keyboard()->keymap_size());
  grab->send_repeat_info(this->current_keyboard()->repeat_rate(),
      this->current_keyboard()->repeat_delay());
  grab->send_modifiers(this->current_keyboard()->mods().depressed,
      this->current_keyboard()->mods().latched,
      this->current_keyboard()->mods().locked,
      this->current_keyboard()->mods().group);
}
void ServerKeyboard::try_remove_keyboard_grab(
    std::shared_ptr<proto::wlr::input_method::KeyboardGrab> grab) {
  if (this->keyboard_grab_.lock() != grab) {
    return;
  }
  this->keyboard_grab_.reset();
  // if keymap/modifier is changed while grab is active,
  // it is not sent to wl_keyboard
  server::get().seat.foreach (
      [this](std::shared_ptr<client_seat::ClientSeat>& client_seat) {
        this->send_initial_events(client_seat);
      });
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
  if (this->keyboard_exclusive_surface_.lock()) {
    return false;
  }
  if (this->focused_surface_.lock() == surface.lock()) {
    return false;
  }
  if (auto focused_surface = this->focused_surface_.lock()) {
    const auto& client_seat =
        server::get().seat.get_client_seat(focused_surface->client());
    if (client_seat) {
      client_seat.value()->keyboard.send_leave(focused_surface);
      client_seat.value()->text_input.send_leave(focused_surface);
    }
  }
  this->focused_surface_     = surface;
  const auto focused_surface = surface.lock();
  if (!focused_surface) {
    return true;
  }
  auto current_keyboard = this->current_keyboard_.lock();
  if (!current_keyboard) {
    set_current_keyboard(this->external_keyboard_);
    current_keyboard = this->external_keyboard_;
  }
  const auto& client_seat =
      server::get().seat.get_client_seat(focused_surface->client());
  if (client_seat) {
    auto pressed_keys = current_keyboard->get_pressed_keys();
    client_seat.value()->keyboard.send_enter(focused_surface, &pressed_keys);
    client_seat.value()->text_input.send_enter(focused_surface);
    client_seat.value()->keyboard.set_current_keyboard(current_keyboard);
  }
  return true;
}
void ServerKeyboard::set_current_focused_surface_as_exclusive() {
  this->keyboard_exclusive_surface_ = this->focused_surface_;
}
}  // namespace yabe::core::server_seat
