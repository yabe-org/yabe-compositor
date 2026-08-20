#pragma once

#include <functional>

#include "common/common.hpp"
#include "core/setting/engine.hpp"

namespace yabe::core::setting::fields {
/// negative value is invalid
using DispatcherId = int;

class Dispatchers {
 public:
  DISABLE_MOVE_AND_COPY(Dispatchers);
  explicit Dispatchers() = default;
  ~Dispatchers()         = default;

  void initialize(Engine* engine);
  void reset();
  void set_executer(const std::function<int(const char* const *)>&& execvp);

  [[nodiscard]] bool is_valid(DispatcherId id) const {
    return id >= 0 && static_cast<std::size_t>(id) < this->dispatchers_.size();
  }
  void dispatch(DispatcherId id);

 private:
  static DispatcherId next_dispatcher_id;
  static DispatcherId get_next_dispatcher_id();

  std::function<int(const char* const *)> execvp_;
  std::vector<std::function<void()>>      dispatchers_;
};
}  // namespace yabe::core::setting::fields
