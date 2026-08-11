#pragma once

#include <wayland-server-core.h>

#include <memory>

#include "core/data-device/data-offer.hpp"

namespace yabe::proto::wayland::data_device_manager::data_offer {
std::shared_ptr<core::data_device::DataOffer> create(
    wl_client* client, std::shared_ptr<core::data_device::DataSource> source);
}
