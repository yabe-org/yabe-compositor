#pragma once

#include <type_traits>
#include <vector>

namespace yabe::common {
template <typename T>
struct IsVector : std::false_type {};

template <typename T, typename Allocator>
struct IsVector<std::vector<T, Allocator>> : std::true_type {
  using value_type = T;
};
}  // namespace yabe::common
