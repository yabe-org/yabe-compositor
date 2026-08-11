#include "server-data-device.hpp"

#include "core/server.hpp"
#include "protocols/wayland/data-device/data-offer.hpp"

namespace yabe::core::server_seat {
void ServerDataDevice::set_selection(
    std::shared_ptr<data_device::DataDevice> data_device) {  // NOLINT
  this->selected_ = data_device;
  auto keyboard_focused_surface =
      server::get().seat.keyboard.get_focused_surface().lock();
  if (!keyboard_focused_surface) {
    return;
  }
  this->send_selection(keyboard_focused_surface->client());
}

void ServerDataDevice::send_selection(wl_client* client) {
  auto client_seat = server::get().seat.get_client_seat(client);
  if (!client_seat) {
    return;
  }

  auto selected = this->selected_.lock();
  if (!selected || !selected->data_source().lock()) {
    client_seat.value()->data_device.send_selection(std::nullopt);
    return;
  }
  const auto selected_source = selected->data_source().lock();
  const auto offer = proto::wayland::data_device_manager::data_offer::create(
      client, selected_source);
  client_seat.value()->data_device.send_data_offer(offer);
  selected_source->for_each_mime([&offer](const std::string& mime) {
    offer->send_offer(mime);
  });
  client_seat.value()->data_device.send_selection(offer);
}
}  // namespace yabe::core::server_seat
