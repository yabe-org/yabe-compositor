#include "exported.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-foreign-protocol.h>

#include <format>
#include <memory>
#include <random>
#include <string>
#include <utility>

#include "core/server.hpp"
#include "core/surface.hpp"
#include "imported.hpp"  // IWYU pragma: keep

namespace yabe::proto::xdg::foreign::exported {
Exported::Exported(std::string handle, std::shared_ptr<core::Surface>& surface)
    : handle_(std::move(handle))
    , surface_(surface)
    , surface_destroyed_listener_([this](std::nullptr_t*) {
      this->invalidate_imported();
    }) {
  surface->add_listener_destroyed(this->surface_destroyed_listener_);
}
Exported::~Exported() {
  this->invalidate_imported();
}
void Exported::add_imported(
    const std::shared_ptr<imported::Imported>& imported) {
  this->imported_.emplace_back(imported);
}
void Exported::remove_imported(
    const std::shared_ptr<imported::Imported>& imported) {
  this->imported_.remove_if(
      [imported](const std::weak_ptr<imported::Imported>& e) {
        return e.lock() == imported;
      });
}
void Exported::invalidate_imported() {
  for (auto& e : this->imported_) {
    if (auto imported = e.lock()) {
      imported->invalidate_parent();
    }
  }
  this->imported_.clear();
}

namespace {
// NOLINTBEGIN(cert-err58-cpp, bugprone-throwing-static-initialization)
std::random_device generate_seed;
std::mt19937       random_engine(generate_seed());
// NOLINTEND(cert-err58-cpp, bugprone-throwing-static-initialization)
constexpr size_t   kBytes = 8;
void               generate_handle(std::string& handle) {
  std::uniform_int_distribution<int> get_rand(0, 0xFF);
  auto                               out = handle.begin();
  for (size_t i = 0; i < kBytes; ++i) {
    out = std::format_to(out, "{:02x}", get_rand(random_engine));
  }
}

constexpr struct zxdg_exported_v2_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};
void handle_destruction(wl_resource* resource) {
  auto* self   = static_cast<Exported*>(wl_resource_get_user_data(resource));
  auto  handle = self->handle();
  core::server::get().exported_map.erase(handle);
}
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    std::shared_ptr<core::Surface>& surface) {
  wl_resource* resource =
      wl_resource_create(client, &zxdg_exported_v2_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto&       map = core::server::get().exported_map;
  std::string handle(kBytes * 2, '\0');
  do {
    generate_handle(handle);
  } while (map.find(handle) != map.end());
  auto self = std::make_shared<Exported>(handle, surface);
  core::server::get().exported_map.emplace(handle, self);
  wl_resource_set_implementation(
      resource, &kImpl, self.get(), handle_destruction);
  zxdg_exported_v2_send_handle(resource, handle.c_str());
}
}  // namespace yabe::proto::xdg::foreign::exported
