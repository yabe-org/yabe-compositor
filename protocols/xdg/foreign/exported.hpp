#pragma once

#include <wayland-server-core.h>

#include <cstddef>
#include <list>

#include "common/common.hpp"
#include "common/signal.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg::foreign::imported {
class Imported;
}

namespace yabe::proto::xdg::foreign::exported {
class Exported {
 public:
  DISABLE_MOVE_AND_COPY(Exported);
  Exported(std::string handle, std::shared_ptr<core::Surface>& surface);
  ~Exported();

  void add_imported(const std::shared_ptr<imported::Imported>& imported);
  void remove_imported(const std::shared_ptr<imported::Imported>& imported);

  [[nodiscard]] const std::string& handle() const {
    return this->handle_;
  }
  [[nodiscard]] std::weak_ptr<core::Surface>& surface() {
    return this->surface_;
  }

 private:
  void invalidate_imported();

  std::string                                  handle_;
  std::weak_ptr<core::Surface>                 surface_;
  common::Listener<std::nullptr_t*>            surface_destroyed_listener_;
  std::list<std::weak_ptr<imported::Imported>> imported_;
};

void create(wl_client* client, int version, uint32_t id,
    std::shared_ptr<core::Surface>& surface);
}  // namespace yabe::proto::xdg::foreign::exported
