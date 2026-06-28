#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void yabe_compositor_start(void (*print_log)(const char*), int fd);
void yabe_compositor_terminate();
void yabe_compositor_draw(const float* vp_mat);

#ifdef __cplusplus
}
#endif
