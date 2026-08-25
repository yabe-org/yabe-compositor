#include "buffer.hpp"

#include <wayland-server.h>

namespace yabe::core {
Buffer::Buffer(wl_resource* buffer_resource) {
  this->set_resource(buffer_resource);
}
Buffer::Buffer(const Buffer& other) : Buffer(other.resource_.get()) {
}
Buffer& Buffer::operator=(const Buffer& other) {
  if (this != &other) {
    this->set_resource(other.resource_.get());
  }
  return *this;
}
Buffer::~Buffer() {
  this->set_resource(nullptr);
}

void Buffer::set_resource(wl_resource* resource) {
  if (this->shm_buffer_) {
    wl_shm_buffer_unref(this->shm_buffer_);
    this->shm_buffer_ = nullptr;
  }
  this->resource_.set(resource);
  if (!this->resource_.get()) {
    return;
  }
  this->shm_buffer_ = wl_shm_buffer_get(this->resource_.get());
  if (!this->shm_buffer_) {
    return;
  }
  wl_shm_buffer_ref(this->shm_buffer_);
}
void Buffer::unref_shm_buffer() {
  if (!this->shm_buffer_) {
    return;
  }
  wl_shm_buffer_unref(this->shm_buffer_);
  this->shm_buffer_ = nullptr;
}
}  // namespace yabe::core
