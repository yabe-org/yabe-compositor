#include "keyboard.hpp"

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#include <cstring>

#include "common/common.hpp"
#include "core/server.hpp"

namespace yabe::core {
namespace {
void log_for_xkbcommon(struct xkb_context* /*context*/,
    enum xkb_log_level level, const char* format, va_list args) {
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
}  // namespace

Keyboard::Keyboard() {
  this->xkb_context_ = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
  if (!this->xkb_context_) {
    return;
  }
  xkb_context_set_log_level(this->xkb_context_, XKB_LOG_LEVEL_DEBUG);
  xkb_context_set_log_fn(this->xkb_context_, log_for_xkbcommon);
}
Keyboard::~Keyboard() {
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

void Keyboard::set_keymap(uint32_t format, int original_fd, size_t size) {
  int fd = fcntl(original_fd, F_DUPFD_CLOEXEC, 0);
  if (fd < 0) {
    LOG_DBG("Failed to duplicate given fd");
    return;
  }
  xkb_keymap* new_keymap = nullptr;
  {
    void* ptr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      LOG_ERR("Failed to map xkb keymap shared memory");
      close(fd);
      return;
    }
    new_keymap = xkb_keymap_new_from_buffer(this->xkb_context_,
        static_cast<const char*>(ptr), size,
        static_cast<xkb_keymap_format>(format), XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(ptr, size);
  }
  if (!new_keymap) {
    LOG_DBG("Failed to create keymap");
    close(fd);
    return;
  }
  auto* new_state = xkb_state_new(new_keymap);
  if (!new_state) {
    LOG_DBG("Failed to create state");
    close(fd);
    xkb_keymap_unref(new_keymap);
    return;
  }

  if (this->xkb_state_) {
    xkb_state_unref(this->xkb_state_);
  }
  if (this->xkb_keymap_) {
    xkb_keymap_unref(this->xkb_keymap_);
  }
  if (this->keymap_fd_ >= 0) {
    close(this->keymap_fd_);
  }
  this->xkb_keymap_  = new_keymap;
  this->xkb_state_   = new_state;
  this->keymap_fd_   = fd;
  this->keymap_size_ = size;
  server::get().seat.keyboard.handle_keyboard_keymap_changed(
      this->shared_from_this());
}
void Keyboard::set_repeat_info(int32_t rate, int32_t delay) {
  this->repeat_rate_  = rate;
  this->repeat_delay_ = delay;
  server::get().seat.keyboard.handle_keyboard_repeat_info_changed(
      this->shared_from_this());
}

bool Keyboard::update_xkb_state(uint32_t key, bool is_pressed) {
  if (!this->is_valid()) {
    return false;
  }
  xkb_state_update_key(
      this->xkb_state_, key + 8, is_pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
  ModMasks mods = {
      .depressed =
          xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_DEPRESSED),
      .latched =
          xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_LATCHED),
      .locked =
          xkb_state_serialize_mods(this->xkb_state_, XKB_STATE_MODS_LOCKED),
      .group = xkb_state_serialize_layout(
          this->xkb_state_, XKB_STATE_LAYOUT_EFFECTIVE),
  };
  if (this->mods_ == mods) {
    return false;
  }
  this->mods_ = mods;
  return true;
}
void Keyboard::handle_key(uint32_t key, bool is_pressed) {
  if (!this->is_valid()) {
    return;
  }
  const auto it =
      std::find(this->pressed_keys_.begin(), this->pressed_keys_.end(), key);
  if (is_pressed) {
    if (it == this->pressed_keys_.end()) {
      this->pressed_keys_.push_back(key);
    }
  } else if (it != this->pressed_keys_.end()) {
    this->pressed_keys_.erase(it);
  }
}
void Keyboard::handle_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group) {
  if (!this->is_valid()) {
    return;
  }
  this->mods_.depressed = mods_depressed;
  this->mods_.latched   = mods_latched;
  this->mods_.locked    = mods_locked;
  this->mods_.group     = group;
}

bool Keyboard::is_key_pressed(xkb_keysym_t keysym) const {
  if (!this->is_valid()) {
    return false;
  }
  // FIXME: this does not handle mods.group
  return std::ranges::any_of(this->pressed_keys_,  //
      [this, keysym](uint32_t keycode) {
        auto current_sym = this->get_one_sym(keycode + 8);
        // convert upper because, e.g. in XKB_KEY_A,
        // 'A' is upper char and feel more like constant
        return xkb_keysym_to_upper(current_sym) == keysym;
      });
}
bool Keyboard::is_mod_pressed(ModKeys mod) const {
  if (!this->is_valid()) {
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
  // same behavior with xkb_state_mod_name_is_active(XKB_STATE_MODS_EFFECTIVE)
  const auto active =
      this->mods_.depressed | this->mods_.latched | this->mods_.locked;
  const auto mask = xkb_keymap_mod_get_mask(this->xkb_keymap_, name);
  return (active & mask) != 0;
}
xkb_mod_mask_t Keyboard::try_convert_to_mod_mask(const char* mod_name) const {
  if (!this->is_valid()) {
    return 0;
  }
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): `return` cannot be replaced with lambda function
#define CASE(str, mod_def)                                                     \
  if (strcasecmp(mod_name, str) == 0) {                                        \
    return xkb_keymap_mod_get_mask(this->xkb_keymap_, mod_def);                \
  }
  CASE("ctrl", XKB_MOD_NAME_CTRL)
  CASE("alt", XKB_MOD_NAME_ALT)
  CASE("shift", XKB_MOD_NAME_SHIFT)
  CASE("super", XKB_MOD_NAME_LOGO)
  CASE("caps", XKB_MOD_NAME_CAPS)
  CASE("num", XKB_MOD_NAME_MOD2)
  CASE("mod3", XKB_MOD_NAME_MOD3)
  CASE("mod5", XKB_MOD_NAME_MOD5)
#undef CASE
  return 0;
}

wl_array Keyboard::get_pressed_keys() const {
  return wl_array{
      .size  = this->pressed_keys_.size() * sizeof(uint32_t),
      .alloc = 0,
      .data  = const_cast<uint32_t*>(this->pressed_keys_.data()),  // NOLINT
  };
}
}  // namespace yabe::core
