#pragma once

#include <memory>

#include "core/surface.hpp"

namespace yabe::proto::xdg_shell::xdg_surface {
class XdgSurfaceRole : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurfaceRole);
  explicit XdgSurfaceRole(
      wl_resource* xdg_surface, std::weak_ptr<core::Surface>&& wl_surface);
  ~XdgSurfaceRole() override = default;

  [[nodiscard]] std::shared_ptr<core::Surface> surface() const {
    return this->wl_surface_.lock();
  }

  void commit() override;
  void set_activated(bool is_activated) override;

  // FIXME: only xdg_toplevel for now
  void set_xdg_role_object(wl_resource* role_object);

 private:
  std::weak_ptr<core::Surface> wl_surface_;
  wl_resource*                 xdg_surface_;
  wl_resource* xdg_role_object_ = nullptr;  // xdg_toplevel or xdg_popup
  bool         is_activated_    = false;
  bool         is_first_commit_ = true;
};
}  // namespace yabe::proto::xdg_shell::xdg_surface
