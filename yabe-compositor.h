#pragma once

#ifdef __cplusplus
extern "C" {
#include <cstdint>
#elif
#include <stdbool.h>
#include <stdint.h>
#endif

void yabe_compositor_start(void (*print_log)(const char*), int fd);
void yabe_compositor_terminate();
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

#ifdef __cplusplus
}
#endif
