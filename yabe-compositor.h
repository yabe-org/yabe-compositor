#pragma once

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#elif
#include <stdbool.h>
#include <stdint.h>
#endif

void yabe_compositor_init(void (*print_log)(const char*));
/// @param fd if fd is -1, the compositor will create a socket
void yabe_compositor_register_socket_fd(int fd);
void yabe_compositor_start();
void yabe_compositor_terminate();
void yabe_compositor_add_client_fd(int fd);
void yabe_compositor_draw(const float* vp_mat);

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
/// @param rate   characters per second
/// @param delay  delay in milliseconds before repeat starts
void yabe_compositor_repeat_info(int32_t rate, int32_t delay);
/// send keyboard modifiers to all clients
void yabe_compositor_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group);

#ifdef __cplusplus
}
#endif
