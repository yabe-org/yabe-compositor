#include "data-device.hpp"

#include <wayland-server-protocol.h>

#include "core/server-seat/pointer-grab/drag-and-drop.hpp"
#include "core/server.hpp"

namespace yabe::core::data_device {
DataDevice::DataDevice(wl_resource* resource) : resource_(resource) {
}

void DataDevice::start_drag(std::optional<std::shared_ptr<DataSource>> source,
    std::optional<std::weak_ptr<Surface>> icon_surface) {
  if (source.has_value()) {
    this->source_ = source.value();
  } else {
    this->source_ = std::weak_ptr<DataSource>();
  }
  if (icon_surface.has_value()) {
    server::get().ray_cursor.set_drag_and_drop_icon(
        std::weak_ptr(*icon_surface));
  }

  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::DragAndDropPointerGrab::start(
          std::weak_ptr(this->source_.lock())));
}

void DataDevice::set_selection(std::shared_ptr<DataSource> source) {  // NOLINT
  this->source_ = source;
  server::get().seat.data_device.set_selection(this->shared_from_this());
}
}  // namespace yabe::core::data_device
