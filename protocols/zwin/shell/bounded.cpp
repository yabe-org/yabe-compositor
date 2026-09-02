#include "bounded.hpp"

#include <zwin-shell-protocol.h>

#include "common/convert-wl-array.hpp"
#include "core/server-seat/pointer-grab/move.hpp"
#include "core/server.hpp"
#include "protocols/zwin/region.hpp"

namespace yabe::proto::zwin::shell {
Bounded::Bounded(wl_resource*             resource,
    const std::shared_ptr<VirtualObject>& virtual_object,
    const glm::vec3&                      half_size)
    : resource_(resource)
    , virtual_object_(virtual_object)
    , half_size_(half_size) {
}
Bounded::~Bounded() {
  this->unmap();
}

void Bounded::set_region(std::optional<compositor::Region*> region) {
  if (region) {
    this->region_ = **region;  // deref optional, then pointer
  } else {
    this->region_ = std::nullopt;
  }
}
void Bounded::start_move() {
  if (!this->scene_node_) {
    return;
  }
  core::server::get().seat.pointer.start_grab(
      core::server_seat::pointer_grab::MovePointerGrab::start(
          *this->scene_node_));
}

void Bounded::map() {
  if (this->scene_node_) {
    return;
  }
  const auto virtual_object = this->virtual_object_.lock();
  if (!virtual_object) {
    return;
  }
  this->scene_node_ =
      std::make_shared<core::scene::BoundedNode>(this->shared_from_this());
  core::server::get().workspace.current_scene().insert(
      this->scene_node_.value());
}
void Bounded::unmap() {
  if (!this->scene_node_) {
    return;
  }
  core::server::get().workspace.current_scene().erase(
      this->scene_node_.value());
}
void Bounded::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  const auto virtual_object = this->virtual_object_.lock();
  if (!virtual_object) {
    return;
  }
  virtual_object->draw(proj_mat, view_mat, model_mat);
}

namespace {
constexpr struct zwn_bounded_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .ack_configure =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            uint32_t /*serial*/) {
          // todo?
        },
    .set_title =
        [](wl_client* /*client*/, wl_resource* /*resource*/,
            const char* /*title*/) {
          // todo?
        },
    .set_region =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* region_resource) {
          const auto self =
              core::server::get().resources.get<Bounded>(resource);
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
    .move =
        [](wl_client* /*client*/, wl_resource* resource, wl_resource* /*seat*/,
            uint32_t /*serial*/) {
          const auto self =
              core::server::get().resources.get<Bounded>(resource);
          if (self) {
            self->start_move();
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_bounded(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object,
    wl_array*                             half_size_array) {
  glm::vec3 half_size;
  if (!common::from_wl_array(half_size_array, &half_size)) {
    wl_client_post_implementation_error(
        client, "zwn_shell.get_bounded: half_size array has invalid size");
    return;
  }
  wl_resource* resource =
      wl_resource_create(client, &zwn_bounded_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self =
      std::make_shared<Bounded>(resource, virtual_object, half_size);
  virtual_object->set_parent(self);
  core::server::get().resources.add(resource, self);
  zwn_bounded_send_configure(
      resource, half_size_array, core::server::get().next_serial());
}
}  // namespace yabe::proto::zwin::shell
