#pragma once

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#elif
#include <stdbool.h>
#include <stdint.h>
#endif

void     yabe_compositor_init(void (*print_log)(const char*),
    /// @retval nullptr if failed, otherwise keymap string in
    /// XKB_KEYMAP_FORMAT_TEXT_V1, which pointer should alive longer until the
    /// next call of this function, and the memory is owned by the caller
    const char* (*create_keymap)(const char* rules, const char* model,
        const char* layout, const char* variant, const char* options),
    int /*pid*/ (*execvp)(const char* const * /* NULL-terminated argv */));
void     yabe_compositor_load_config(const char* config_contents);
/// @param fd if fd is -1, the compositor will create a socket
/// @return zero if fd != -1, otherwise index of socket name `wayland-%u`
/// (e.g. if it creates `wayland-1`, return 1)
uint32_t yabe_compositor_register_socket_fd(int fd);
void     yabe_compositor_start();
void     yabe_compositor_terminate();
void     yabe_compositor_add_client_fd(int fd);
void     yabe_compositor_draw(const float* proj_mat, const float* view_mat);

/// @param dx right is positive
/// @param dy up is positive
void yabe_compositor_move_pointer(float dx, float dy);
/// @param button same as the button code in the linux/input-event-codes.h
///               such as BTN_LEFT
/// @param is_pressed true if the button is pressed, false if released
void yabe_compositor_button(uint32_t button, bool is_pressed);
/// @param axis 0: vertical, 1: horizontal
void yabe_compositor_axis(uint32_t axis, float value);
/// @param key same as the key code in the linux/input-event-codes.h
///            such as KEY_A
/// @param is_pressed true if the key is pressed, false if released
void yabe_compositor_key(uint32_t key, bool is_pressed);
/// @param format e.g. WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 (1)
/// @param fd     file descriptor of the shared memory containing the keymap
/// @param size   size of the keymap in bytes
void yabe_compositor_keymap(uint32_t format, int32_t fd, uint32_t size);

#ifdef __cplusplus
}
#endif
