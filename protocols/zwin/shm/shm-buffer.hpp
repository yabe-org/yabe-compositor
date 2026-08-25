#pragma once

#include <sys/types.h>
#include <wayland-server.h>
#include <zwin-protocol.h>

#include "shm-pool.hpp"

namespace yabe::proto::zwin::shm {
struct ShmBuffer;

ShmBuffer* new_buffer(
    wl_client* client, uint32_t id, ShmPool* pool, off_t size, off_t offset);
void       buffer_begin_access(ShmBuffer* buffer);
void       buffer_end_access(ShmBuffer* buffer);
ShmBuffer* get_buffer(wl_resource* resource);
void*      get_buffer_data(ShmBuffer* buffer);
ssize_t    get_buffer_size(ShmBuffer* buffer);
}  // namespace yabe::proto::zwin::shm
