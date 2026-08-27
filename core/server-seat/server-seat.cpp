#include "server-seat.hpp"

#include <utility>

#include "common/common.hpp"

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
}  // namespace yabe::core::server_seat
