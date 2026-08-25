#include "server-seat.hpp"

#include <utility>

#include "common/common.hpp"
#include "core/server.hpp"

namespace yabe::core::server_seat {
void ServerSeat::create_client_seat(wl_client* client) {
  const auto it = this->client_seats_.find(client);
  if (it != this->client_seats_.end()) {
    return;
  }
  auto client_seat = std::make_shared<client_seat::ClientSeat>();
  this->client_seats_.emplace(client, std::move(client_seat));
  LOG_DBG("create client_seat for client");
}
void ServerSeat::remove_client_seat(wl_client* client) {
  LOG_DBG("remove client_seat for client");
  this->client_seats_.erase(client);
}

// NOLINTBEGIN(performance-unnecessary-value-param): without this,
// even if caller wants to set current active toplevel inactive,
// it must create an empty shared_ptr but it is cumbersome
void ServerSeat::set_active_toplevel(std::shared_ptr<Toplevel> toplevel) {
  const auto current_focused_toplevel = this->active_toplevel_.lock();
  if (current_focused_toplevel == toplevel) {
    return;
  }
  if (current_focused_toplevel) {
    current_focused_toplevel->set_activated(false);
  }
  this->active_toplevel_ = toplevel;
  if (toplevel) {
    toplevel->set_activated(true);
    auto node = toplevel->surface().lock()->surface_node();
    if (node.has_value()) {
      server::get().scene.raise_to_front(
          toplevel->surface().lock()->surface_node().value());
    }
  }
}
// NOLINTEND(performance-unnecessary-value-param)

void ServerSeat::try_active_mapped_toplevel(
    std::shared_ptr<Toplevel> toplevel) {
  if (this->active_toplevel_.lock()) {
    return;
  }
  if (this->keyboard.get_focused_surface().lock()) {
    return;
  }
  this->data_device.send_selection(toplevel->surface().lock()->client());
  this->keyboard.set_focused_surface(toplevel->surface());
  this->set_active_toplevel(std::move(toplevel));
}
}  // namespace yabe::core::server_seat
