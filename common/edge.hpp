#pragma once

#include <bitset>
#include <cstdint>

namespace yabe::common {
class Edge {
 public:
  Edge()                       = default;
  Edge(const Edge&)            = default;
  Edge& operator=(const Edge&) = default;
  Edge(Edge&&)                 = delete;
  Edge& operator=(Edge&&)      = delete;
  ~Edge()                      = default;

  enum Pos : uint8_t {
    TOP    = 0,
    BOTTOM = 1,
    LEFT   = 2,
    RIGHT  = 3,
  };

  template <typename T>
  void set(T value);

  bool at(Pos pos) {
    return this->edges_.test(static_cast<size_t>(pos));
  }

 private:
  std::bitset<4> edges_;
};

template <>
inline void Edge::set(Pos value) {
  this->edges_.set(static_cast<size_t>(value));
}
}  // namespace yabe::common
