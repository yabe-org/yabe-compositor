#pragma once

#include <memory>
#include <unordered_map>

#include "common/common.hpp"
#include "core/client-seat/client-seat.hpp"
#include "core/server-seat/server-data-device.hpp"
#include "core/server-seat/server-keyboard.hpp"
#include "core/server-seat/server-pointer.hpp"
#include "core/toplevel.hpp"

namespace yabe::core::server_seat {
class ServerSeat {
 public:
  DISABLE_MOVE_AND_COPY(ServerSeat);
  explicit ServerSeat() = default;
  ~ServerSeat()         = default;

  void create_client_seat(wl_client* client);
  void remove_client_seat(wl_client* client);

  [[nodiscard]] std::optional<std::shared_ptr<client_seat::ClientSeat>>
  get_client_seat(wl_client* client) {
    auto it = this->client_seats_.find(client);
    if (it == this->client_seats_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  template <typename Func>
  void foreach (Func func) {
    for (auto& [client, client_seat] : this->client_seats_) {
      if (client_seat) {
        func(*client_seat);
      }
    }
  }

  ServerKeyboard   keyboard;
  ServerPointer    pointer;
  ServerDataDevice data_device;

 private:
  std::unordered_map<wl_client*, std::shared_ptr<client_seat::ClientSeat>>
      client_seats_;
};
}  // namespace yabe::core::server_seat
