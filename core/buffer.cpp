#include "buffer.hpp"

#include <wayland-server.h>

#include "wayland-util.h"

namespace yabe::core {
Buffer::Buffer(wl_resource* buffer_resource) {
  wl_list_init(&this->resource_destroy_listener_.link);
  this->resource_destroy_listener_.notify = [](wl_listener* listener,
                                                void* /*data*/) {
    // NOLINTBEGIN
    Buffer* buffer =
        wl_container_of(listener, buffer, resource_destroy_listener_);
    // NOLINTEND
    buffer->set_resource(nullptr);
  };
  this->set_resource(buffer_resource);
}
Buffer::Buffer(const Buffer& other) : Buffer(other.resource_) {
}
Buffer& Buffer::operator=(const Buffer& other) {
  if (this == &other) {
    return *this;
  }
  this->set_resource(other.resource_);
  return *this;
}
Buffer::~Buffer() {
  this->set_resource(nullptr);
}

void Buffer::set_resource(wl_resource* resource) {
  if (this->resource_ != nullptr) {
    wl_list_remove(&this->resource_destroy_listener_.link);
  }
  if (this->shm_buffer_) {
    wl_shm_buffer_unref(this->shm_buffer_);
    this->shm_buffer_ = nullptr;
  }
  this->resource_ = resource;
  if (this->resource_ == nullptr) {
    return;
  }
  this->shm_buffer_ = wl_shm_buffer_get(resource);
  wl_shm_buffer_ref(this->shm_buffer_);
  wl_resource_add_destroy_listener(resource, &this->resource_destroy_listener_);
}
void Buffer::unref_shm_buffer() {
  if (!this->shm_buffer_) {
    return;
  }
  wl_shm_buffer_unref(this->shm_buffer_);
  this->shm_buffer_ = nullptr;
}
}  // namespace yabe::core
