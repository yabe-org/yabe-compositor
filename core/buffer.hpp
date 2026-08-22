#pragma once

#include <wayland-server-core.h>

namespace yabe::core {
// If resource is non-null, shm_buffer is also non-null
// If resource is     null, shm_buffer is also     null
class Buffer {
 public:
  explicit Buffer(wl_resource* buffer_resource);
  ~Buffer();
  Buffer(const Buffer&);
  Buffer& operator=(const Buffer&);
  Buffer(Buffer&&)            = delete;
  Buffer& operator=(Buffer&&) = delete;

  void set_resource(wl_resource* resource);
  void unref_shm_buffer();

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] wl_shm_buffer* shm_buffer() const {
    return this->shm_buffer_;
  }

  bool operator==(const Buffer& other) const {
    return this->resource_ == other.resource_;
  }

 private:
  wl_resource*   resource_   = nullptr;  // nullable
  wl_shm_buffer* shm_buffer_ = nullptr;
  wl_listener    resource_destroy_listener_;
};
}  // namespace yabe::core
