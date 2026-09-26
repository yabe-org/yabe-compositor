#pragma once

#include <wayland-server-core.h>

#include <memory>
#include <wlr/input-method/input-method.hpp>

#include "common/box.hpp"
#include "common/common.hpp"
#include "core/surface.hpp"

namespace yabe::proto::wlr::input_method {
class InputPopup : public core::SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(InputPopup);
  InputPopup(
      wl_resource* resource, const std::shared_ptr<core::Surface>& surface);
  ~InputPopup() override;

  void commit() override {
  }
  [[nodiscard]] std::shared_ptr<core::Toplevel> get_toplevel() override {
    return {};
  }

  void send_text_input_rectangle(const common::Box2D& cursor_rect);
  [[nodiscard]] std::weak_ptr<core::Surface> surface() {
    return this->surface_.lock();
  }

 private:
  wl_resource*                 resource_;
  std::weak_ptr<core::Surface> surface_;
};
void create_input_popup(wl_client* client, int version, uint32_t id,
    std::shared_ptr<InputMethod>&   input_method,
    std::shared_ptr<core::Surface>& surface);
}  // namespace yabe::proto::wlr::input_method
