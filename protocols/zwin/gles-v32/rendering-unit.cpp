#include "rendering-unit.hpp"

#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>

#include "base-technique.hpp"
#include "common/common.hpp"
#include "core/server.hpp"
#include "protocols/zwin/virtual-object.hpp"

namespace yabe::proto::zwin::gles_v32 {
bool RenderingUnit::set_technique(
    const std::shared_ptr<BaseTechnique>& technique) {
  const std::lock_guard lock(this->mutex_);
  if (this->technique_.lock()) {
    return false;
  }
  this->technique_ = technique;
  return true;
}

void RenderingUnit::commit() {
  std::shared_ptr<BaseTechnique> technique;
  {
    const std::lock_guard lock(this->mutex_);
    technique = this->technique_.lock();
  }
  if (technique) {
    technique->commit();
  }
}

void RenderingUnit::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  std::shared_ptr<BaseTechnique> technique;
  {
    const std::lock_guard lock(this->mutex_);
    technique = this->technique_.lock();
  }
  if (technique) {
    technique->draw(proj_mat, view_mat, model_mat);
  }
}

namespace {
constexpr struct zwn_rendering_unit_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_rendering_unit(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_rendering_unit_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<RenderingUnit>();
  virtual_object->add_rendering_unit(self);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::gles_v32
