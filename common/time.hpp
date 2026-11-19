#pragma once

#include <cstdint>
#include <ctime>

namespace yabe::common {
inline uint32_t now_msec() {
  std::timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
    return -1;
  }
  return (static_cast<int64_t>(ts.tv_sec) * 1000) + (ts.tv_nsec / 1'000'000);
}

inline uint64_t now_usec() {
  std::timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
    return -1;
  }
  return (static_cast<uint64_t>(ts.tv_sec) * 1'000'000) +
         (static_cast<uint64_t>(ts.tv_nsec) / 1'000);
}
}  // namespace yabe::common
