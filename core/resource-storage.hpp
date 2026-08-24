#pragma once

#include <wayland-server-core.h>

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "common/common.hpp"

namespace yabe::core {
class ResourceStorage {
 public:
  ResourceStorage() = default;
  DISABLE_MOVE_AND_COPY(ResourceStorage);
  ~ResourceStorage() = default;

  template <typename T>
  void add(wl_resource* resource, const std::shared_ptr<T>& ptr) {
    this->storage_.insert_or_assign(
        resource, Entry{ptr, std::type_index(typeid(T))});
  }

  template <typename T>
  std::shared_ptr<T> get(wl_client* client) {
    for (const auto& [resource, entry] : this->storage_) {
      if (wl_resource_get_client(resource) == client &&
          entry.type == std::type_index(typeid(T))) {
        return std::static_pointer_cast<T>(entry.ptr);
      }
    }
    return nullptr;
  }

  template <typename T>
  std::shared_ptr<T> get(wl_resource* resource) {
    auto it = storage_.find(resource);
    if (it != storage_.end() && it->second.type == std::type_index(typeid(T))) {
      return std::static_pointer_cast<T>(it->second.ptr);
    }
    return nullptr;
  }

  void remove(wl_resource* resource) {
    storage_.erase(resource);
  }

 private:
  struct Entry {
    std::shared_ptr<void> ptr;
    std::type_index       type;
  };
  std::unordered_map<wl_resource*, Entry> storage_;
};
}  // namespace yabe::core
