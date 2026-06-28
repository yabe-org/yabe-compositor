#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void start(void (*print_log)(const char*), int fd);
void draw();

#ifdef __cplusplus
}
#endif
