#pragma once

#include <wayland-server-core.h>

#include <mutex>

#include "common/common.hpp"
#include "common/weak-resource.hpp"

namespace yabe::core {
// If resource is     null, shm_buffer is also     null
// If resource is non-null, shm_buffer might be null (e.g. after uploading)
class Buffer {
 public:
  DISABLE_MOVE_AND_COPY(Buffer);
  explicit Buffer(wl_resource* buffer_resource);
  ~Buffer();

  void set_resource(wl_resource* resource);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_.get();
  }
  [[nodiscard]] wl_shm_buffer* shm_buffer() const {
    return this->shm_buffer_;
  }
  [[nodiscard]] std::lock_guard<std::mutex> lock_resource() {
    return this->resource_.lock();
  }

  bool operator==(const Buffer& other) const {
    return this->resource_ == other.resource_;
  }

 private:
  common::WeakResource resource_;
  wl_shm_buffer*       shm_buffer_ = nullptr;
};
}  // namespace yabe::core
