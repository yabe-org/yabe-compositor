#pragma once

#include <functional>

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/dispatcher-id.hpp"

namespace yabe::core::setting::fields {
using schema::DispatcherId;

class Dispatchers {
 public:
  DISABLE_MOVE_AND_COPY(Dispatchers);
  explicit Dispatchers() = default;
  ~Dispatchers()         = default;

  void initialize(Engine* engine);
  void reset();
  void set_executer(const std::function<int(const char* const *)>&& execvp);

  [[nodiscard]] bool is_valid(DispatcherId id) const {
    return id.id >= 0 &&
           static_cast<std::size_t>(id.id) < this->dispatchers_.size();
  }
  void dispatch(DispatcherId id);

 private:
  static int          next_dispatcher_id;
  static DispatcherId get_next_dispatcher_id();

  std::function<int(const char* const *)> execvp_;
  std::vector<std::function<void()>>      dispatchers_;
};
}  // namespace yabe::core::setting::fields
