#include "buffer.hpp"

#include <wayland-server.h>

#include "wayland-util.h"

namespace yabe::core {
Buffer::Buffer(wl_resource* buffer_resource) {
  wl_list_init(&this->resource_destroy_listener.link);
  this->resource_destroy_listener.notify = [](wl_listener* listener,
                                               void* /*data*/) {
    // NOLINTBEGIN
    Buffer* buffer =
        wl_container_of(listener, buffer, resource_destroy_listener);
    // NOLINTEND
    buffer->set_resource(nullptr);
  };
  this->set_resource(buffer_resource);
}
Buffer::Buffer(const Buffer& other) : Buffer(other.resource) {
}
Buffer& Buffer::operator=(const Buffer& other) {
  this->set_resource(other.resource);
  return *this;
}
Buffer::~Buffer() {
  this->set_resource(nullptr);
}

void Buffer::set_resource(wl_resource* resource) {
  if (this->resource != nullptr) {
    wl_list_remove(&this->resource_destroy_listener.link);
  }
  if (resource == nullptr) {
    this->resource = nullptr;
    return;
  }
  if (this->shm_buffer) {
    wl_shm_buffer_unref(this->shm_buffer);
  }
  this->resource   = resource;
  this->shm_buffer = wl_shm_buffer_get(resource);
  wl_shm_buffer_ref(this->shm_buffer);
  wl_resource_add_destroy_listener(resource, &this->resource_destroy_listener);
}
void Buffer::unref_shm_buffer() {
  if (!this->shm_buffer) {
    return;
  }
  wl_shm_buffer_unref(this->shm_buffer);
  this->shm_buffer = nullptr;
}
}  // namespace yabe::core
