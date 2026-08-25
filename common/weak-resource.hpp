#pragma once

#include <wayland-server-core.h>

namespace yabe::common {
class WeakResource {
 public:
  WeakResource();
  explicit WeakResource(wl_resource* resource);
  WeakResource(const WeakResource&);
  WeakResource(WeakResource&&) = delete;
  WeakResource& operator=(const WeakResource&);
  WeakResource& operator=(WeakResource&&) = delete;
  ~WeakResource();

  void                       set(wl_resource* resource);
  [[nodiscard]] wl_resource* get() const {
    return this->resource_;
  }

  bool operator==(const WeakResource& other) const {
    return this->resource_ == other.resource_;
  }

 private:
  wl_resource* resource_ = nullptr;
  wl_listener  destroy_listener_;
};
}  // namespace yabe::common
