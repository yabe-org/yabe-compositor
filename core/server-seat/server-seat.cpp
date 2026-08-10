#include "server-seat.hpp"

#include <utility>

#include "common/common.hpp"

namespace yabe::core::server_seat {
client_seat::ClientSeat* ServerSeat ::create_client_seat(wl_client* client) {
  auto  client_seat = std::make_unique<client_seat::ClientSeat>();
  auto* ptr         = client_seat.get();
  this->client_seats_.emplace(client, std::move(client_seat));
  LOG_DBG("create client_seat for client");
  return ptr;
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
  this->set_active_toplevel(std::move(toplevel));
}
}  // namespace yabe::core::server_seat
