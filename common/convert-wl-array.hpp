#pragma once

#include <wayland-server-core.h>

#include <cstring>

namespace yabe::common {
/// @return true if succeeded
template <typename T>
inline bool from_wl_array(wl_array* array, T* value) {
  if (array->size != sizeof(T)) {
    return false;
  }
  std::memcpy(value, array->data, array->size);
  return true;
}

template <>
inline bool from_wl_array(wl_array* array, off_t* value) {
  if (array->size != sizeof(int32_t) && array->size != sizeof(int64_t)) {
    return false;
  }
  std::memcpy(value, array->data, array->size);
  return true;
}
}  // namespace yabe::common
