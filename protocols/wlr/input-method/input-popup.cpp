#include "input-popup.hpp"

#include <input-method-protocol.h>
#include <wayland-server-core.h>

#include "core/scene/surface-node.hpp"
#include "core/server.hpp"

namespace yabe::proto::wlr::input_method {
InputPopup::InputPopup(
    wl_resource* resource, const std::shared_ptr<core::Surface>& surface)
    : resource_(resource), surface_(surface) {
}
InputPopup::~InputPopup() {
  if (const auto surface = this->surface_.lock()) {
    surface->unmap();
  }
}
void InputPopup::send_text_input_rectangle(const common::Box2D& cursor_rect) {
  // this sends where is the cursor position relative to the popup
  // send so that popup is at the bottom of the cursor
  // (that is, the cursor pos is above)
  zwp_input_popup_surface_v2_send_text_input_rectangle(this->resource_,  //
      0, -*cursor_rect.y, *cursor_rect.width, *cursor_rect.height);
}

namespace {
constexpr struct zwp_input_popup_surface_v2_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_input_popup(wl_client* client, int version, uint32_t id,
    std::shared_ptr<InputMethod>&   input_method,
    std::shared_ptr<core::Surface>& surface) {
  wl_resource* resource = wl_resource_create(
      client, &zwp_input_popup_surface_v2_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);

  auto self = std::make_shared<InputPopup>(resource, surface);
  surface->set_role(self);
  core::server::get().seat.input_method.set_input_popup(input_method, self);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::wlr::input_method
