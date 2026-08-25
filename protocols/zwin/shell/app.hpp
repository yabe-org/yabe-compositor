#pragma once

#include "common/common.hpp"

namespace yabe::proto::zwin::shell {
class ZwinApp {
 public:
  DISABLE_MOVE_AND_COPY(ZwinApp);
  explicit ZwinApp() = default;
  virtual ~ZwinApp() = default;

  virtual void map()   = 0;
  virtual void unmap() = 0;
};
}  // namespace yabe::proto::zwin::shell
