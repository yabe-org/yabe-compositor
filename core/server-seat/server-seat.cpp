#include "server-seat.hpp"

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
}  // namespace yabe::core::server_seat
