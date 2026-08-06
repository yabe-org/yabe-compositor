#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "common/common.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wayland::subcompositor::subsurface {
class Subsurface : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(Subsurface);
  explicit Subsurface(wl_resource*    resource,
      std::shared_ptr<core::Surface>& surface,
      std::shared_ptr<core::Surface>& parent_surface);
  ~Subsurface() override = default;

  void commit() override;
  void set_activated(bool is_activated) override;

  void set_position(int32_t x, int32_t y);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }

 private:
  core::unit::Pixel x_{0};
  core::unit::Pixel y_{0};
  void              update_position();

  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> surface_;
  std::weak_ptr<core::Surface> parent_surface_;
};

void create(wl_client* client, uint32_t id,
    std::shared_ptr<core::Surface>& surface,
    std::shared_ptr<core::Surface>& parent_surface);
}  // namespace yabe::proto::wayland::subcompositor::subsurface
