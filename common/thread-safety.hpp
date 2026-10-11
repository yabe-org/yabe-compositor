#pragma once

#include <mutex>

#include "common/common.hpp"

// Annotations for Clang Thread Safety Analysis
// (https://clang.llvm.org/docs/ThreadSafetyAnalysis.html)
// They are checked by -Wthread-safety only when compiled with clang
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#if defined(__clang__)
#define CAPABILITY(x)     __attribute__((capability(x)))
#define SCOPED_CAPABILITY __attribute__((scoped_lockable))
#define GUARDED_BY(x)     __attribute__((guarded_by(x)))
#define REQUIRES(...)     __attribute__((requires_capability(__VA_ARGS__)))
#define EXCLUDES(...)     __attribute__((locks_excluded(__VA_ARGS__)))
#define ACQUIRE(...)      __attribute__((acquire_capability(__VA_ARGS__)))
#define RELEASE(...)      __attribute__((release_capability(__VA_ARGS__)))
#else
#define CAPABILITY(x)
#define SCOPED_CAPABILITY
#define GUARDED_BY(x)
#define REQUIRES(...)
#define EXCLUDES(...)
#define ACQUIRE(...)
#define RELEASE(...)
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

namespace yabe::common {
class Mutex;

/// Holds the lock of Mutex until this is destroyed
/// Created by Mutex::lock()
class SCOPED_CAPABILITY LockGuard {
 public:
  DISABLE_MOVE_AND_COPY(LockGuard);
  ~LockGuard() RELEASE();

 private:
  friend class Mutex;
  explicit LockGuard(Mutex& mutex) ACQUIRE(mutex);

  Mutex& mutex_;
};

/// std::mutex of libstdc++ has no annotations, so wrap it
/// Usage: `const auto lock = mutex.lock();`
class CAPABILITY("mutex") Mutex {
 public:
  DISABLE_MOVE_AND_COPY(Mutex);
  Mutex()  = default;
  ~Mutex() = default;

  /// the lock is released when the returned LockGuard is destroyed
  [[nodiscard]] LockGuard lock() ACQUIRE() {
    return LockGuard(*this);
  }

 private:
  friend class LockGuard;
  void lock_raw() ACQUIRE() {
    this->mutex_.lock();
  }
  void unlock_raw() RELEASE() {
    this->mutex_.unlock();
  }

  std::mutex mutex_;
};

inline LockGuard::LockGuard(Mutex& mutex) : mutex_(mutex) {
  this->mutex_.lock_raw();
}
inline LockGuard::~LockGuard() {
  this->mutex_.unlock_raw();
}
}  // namespace yabe::common
