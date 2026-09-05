#include "weak-resource.hpp"

#include <wayland-util.h>

#include <mutex>

namespace yabe::common {
WeakResource::WeakResource() {
  this->destroy_listener_.notify = [](wl_listener* listener, void* /*data*/) {
    // NOLINTNEXTLINE
    WeakResource* self = wl_container_of(listener, self, destroy_listener_);
    self->set(nullptr);
  };
  wl_list_init(&this->destroy_listener_.link);
}
WeakResource::WeakResource(const WeakResource& other) : WeakResource() {
  this->set(other.resource_);
}
WeakResource& WeakResource::operator=(const WeakResource& other) {
  if (this != &other) {
    this->set(other.resource_);
  }
  return *this;
}

WeakResource::WeakResource(wl_resource* resource) : WeakResource() {
  this->set(resource);
}
WeakResource::~WeakResource() {
  wl_list_remove(&this->destroy_listener_.link);
}
void WeakResource::set(wl_resource* resource) {
  const std::lock_guard lock(this->mutex_);
  wl_list_remove(&this->destroy_listener_.link);
  wl_list_init(&this->destroy_listener_.link);
  this->resource_ = resource;
  if (this->resource_ != nullptr) {
    wl_resource_add_destroy_listener(this->resource_, &this->destroy_listener_);
  }
}
}  // namespace yabe::common
