#pragma once

#include <cstdlib>

namespace yabe::proto::zwin::shm {
template <typename T>
inline T zalloc(size_t size) {
  return static_cast<T>(calloc(1, size));
}
}  // namespace yabe::proto::zwin::shm
