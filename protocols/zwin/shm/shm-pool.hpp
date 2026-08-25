#pragma once

#include <sys/types.h>
#include <wayland-server.h>

namespace yabe::proto::zwin::shm {
struct ShmPool {
  wl_resource* resource;
  int          internal_refcount;
  int          external_refcount;
  char*        data;
  ssize_t      size;
  ssize_t      new_size;
  bool         sigbuf_is_impossible;
};

ShmPool* new_pool(wl_client* client, wl_resource* resource, uint32_t id,
    int32_t fd, off_t size, bool sigbuf_is_impossible);
void     unref_pool(ShmPool* pool, bool external);
}  // namespace yabe::proto::zwin::shm
