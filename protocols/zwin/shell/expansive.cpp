#include "expansive.hpp"

#include "core/server.hpp"
#include "protocols/zwin/region.hpp"
#include "zwin-shell-protocol.h"

namespace yabe::proto::zwin::shell {
Expansive::Expansive(
    wl_resource* resource, const std::shared_ptr<VirtualObject>& virtual_object)
    : resource_(resource), virtual_object_(virtual_object) {
}
Expansive::~Expansive() {
  this->unmap();
}

void Expansive::set_region(std::optional<compositor::Region*> region) {
  if (!region) {
    this->region_ = std::nullopt;
    return;
  }
  this->region_ = **region;
}

void Expansive::map() {
  if (this->scene_node_) {
    return;
  }
  const auto virtual_object = this->virtual_object_.lock();
  if (!virtual_object) {
    return;
  }
  this->scene_node_ =
      std::make_shared<core::scene::ExpansiveNode>(this->shared_from_this());
  this->scene_node_.value()->geometry().move_absolute(glm::vec3(0.F));
  core::server::get().scene.insert(this->scene_node_.value());
}
void Expansive::unmap() {
  if (!this->scene_node_) {
    return;
  }
  core::server::get().scene.erase(this->scene_node_.value());
}
void Expansive::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  const auto virtual_object = this->virtual_object_.lock();
  if (!virtual_object) {
    return;
  }
  virtual_object->draw(proj_mat, view_mat, model_mat);
}

namespace {
constexpr struct zwn_expansive_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .set_region =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* region_resource) {
          const auto self =
              core::server::get().resources.get<Expansive>(resource);
          if (!self) {
            return;
          }
          if (!region_resource) {
            self->set_region(std::nullopt);
            return;
          }
          const auto region =
              core::server::get().resources.get<compositor::Region>(
                  region_resource);
          if (region) {
            self->set_region(region.get());
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_expansive(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_expansive_interface, version, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<Expansive>(resource, virtual_object);
  virtual_object->set_parent(self);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::shell
