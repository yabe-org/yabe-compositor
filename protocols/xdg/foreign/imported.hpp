#pragma once

#include <wayland-server-core.h>

#include <list>
#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"

namespace yabe::proto::xdg::foreign::exported {
class Exported;
}

namespace yabe::proto::xdg::foreign::imported {
// Imported should be created even if Exported is invalid;
// then it should be sent destroyed() immediately (create() send it)
class Imported : public std::enable_shared_from_this<Imported> {
 public:
  DISABLE_MOVE_AND_COPY(Imported);
  Imported(wl_resource* resource, std::weak_ptr<exported::Exported>& exported);
  ~Imported();

  void set_parent_of(const std::shared_ptr<core::Surface>& child);
  void invalidate_parent();
  void try_remove_from_exported();

 private:
  void detach_children();

  std::weak_ptr<exported::Exported>       exported_;
  std::list<std::weak_ptr<core::Surface>> children_;
  wl_resource*                            resource_;
};

void create(wl_client* client, int version, uint32_t id,
    std::weak_ptr<exported::Exported>& exported);
}  // namespace yabe::proto::xdg::foreign::imported
