#pragma once

#include <format>

namespace yabe {
/// non-null pointer that is initialized only by start()
extern void (*print_log)(const char*);  // NOLINT
}  // namespace yabe

#define TAGGED_LOG(color, tag, fmt, ...)                                       \
  yabe::print_log(std::format(color "[" tag "|{:12}#{:03}]\x1b[m " fmt,        \
      __FILE_NAME__, __LINE__, ##__VA_ARGS__)                                  \
          .c_str())

// clang-format off
#define LOG_DBG(fmt, ...)  TAGGED_LOG("\x1b[7;2m",  "debug", fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) TAGGED_LOG("\x1b[7;32m", "info ", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) TAGGED_LOG("\x1b[7;33m", "WARN ", fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  TAGGED_LOG("\x1b[7;31m", "ERROR", fmt, ##__VA_ARGS__)
// clang-format on

// NOLINTBEGIN
#define DISABLE_MOVE_AND_COPY(Class)                                           \
  Class(const Class&)            = delete;                                     \
  Class(Class&&)                 = delete;                                     \
  Class& operator=(const Class&) = delete;                                     \
  Class& operator=(Class&&)      = delete
// NOLINTEND
