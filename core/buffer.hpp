#pragma once

#include <wayland-server-core.h>

namespace yabe::core {
struct Buffer {
  explicit Buffer(wl_resource* buffer_resource);
  ~Buffer();
  Buffer(const Buffer&);
  Buffer& operator=(const Buffer&);
  Buffer(Buffer&&)            = delete;
  Buffer& operator=(Buffer&&) = delete;

  wl_resource*   resource   = nullptr;  // nullable
  wl_shm_buffer* shm_buffer = nullptr;
  wl_listener    resource_destroy_listener;

  void set_resource(wl_resource* resource);
  void unref_shm_buffer();

  bool operator==(const Buffer& other) const {
    return this->resource == other.resource;
  }
};
}  // namespace yabe::core
