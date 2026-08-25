#include "virtual-object.hpp"

#include <memory>
#include <mutex>

#include "common/time.hpp"
#include "core/server.hpp"
#include "gles-v32/rendering-unit.hpp"

namespace yabe::proto::zwin {
VirtualObject::VirtualObject(wl_resource* resource) : resource_(resource) {
  LOG_DBG("zwn_virtual_object#{}: created", wl_resource_get_id(resource));
}

void VirtualObject::set_parent(const std::shared_ptr<shell::ZwinApp>& parent) {
  const std::lock_guard lock(this->mutex_);
  this->parent_ = parent;
}
void VirtualObject::add_frame_callback(wl_resource* callback_resource) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.frame_callbacks.emplace_back(callback_resource);
}
void VirtualObject::add_rendering_unit(
    const std::shared_ptr<gles_v32::RenderingUnit>& unit) {
  const std::lock_guard lock(this->mutex_);
  this->rendering_units_.emplace_back(unit);
}

void VirtualObject::commit() {
  {
    std::lock_guard lock(this->mutex_);
    this->current_.frame_callbacks.insert(this->current_.frame_callbacks.end(),
        std::move_iterator(this->pending_.frame_callbacks.begin()),
        std::move_iterator(this->pending_.frame_callbacks.end()));
    this->pending_.frame_callbacks.clear();
  }
  if (const auto parent = this->parent_.lock()) {
    if (this->rendering_units_.empty()) {
      parent->unmap();
    } else {
      parent->map();
    }
  }
  for (auto& weak_unit : this->rendering_units_) {
    if (const auto unit = weak_unit.lock()) {
      unit->commit();
    }
  }
}
void VirtualObject::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  std::vector<wl_resource*> frame_callbacks;
  {
    std::lock_guard lock(this->mutex_);
    for (auto& weak_unit : this->rendering_units_) {
      if (const auto unit = weak_unit.lock()) {
        unit->draw(proj_mat, view_mat, model_mat);
      }
    }

    frame_callbacks.swap(this->current_.frame_callbacks);
  }
  core::server::get().enqueue_wayland_thread_task(
      [frame_callbacks = std::move(frame_callbacks)]() mutable {
        for (auto* callback_resource : frame_callbacks) {
          wl_callback_send_done(callback_resource, common::now_msec());
          wl_resource_destroy(callback_resource);
        }
      });
}

namespace {
constexpr struct zwn_virtual_object_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .commit =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<VirtualObject>(resource);
          if (self) {
            self->commit();
          }
        },
    .frame =
        [](wl_client* client, wl_resource* resource, uint32_t callback) {
          wl_resource* callback_resource = wl_resource_create(client,
              &wl_callback_interface, wl_callback_interface.version, callback);
          if (!callback_resource) {
            wl_client_post_no_memory(client);
            return;
          }

          const auto self =
              core::server::get().resources.get<VirtualObject>(resource);
          if (self) {
            self->add_frame_callback(callback_resource);
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_virtual_object(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_virtual_object_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<VirtualObject>(resource);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin
