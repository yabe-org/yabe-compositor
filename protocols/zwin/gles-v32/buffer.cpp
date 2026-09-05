#include "buffer.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>
#include <utility>

#include "common/common.hpp"
#include "core/server.hpp"
#include "protocols/zwin/shm/shm-buffer.hpp"

namespace yabe::proto::zwin::gles_v32 {
Buffer::~Buffer() {
  if (this->buffer_ == 0) {
    return;
  }
  const GLuint buffer = this->buffer_;
  core::server::get().enqueue_rendering_thread_task([buffer] {
    glDeleteBuffers(1, &buffer);
  });
}

void Buffer::set_data(GLenum target, wl_resource* data, GLenum usage) {
  auto state    = std::make_shared<State>();
  state->target = target;
  state->usage  = usage;
  state->data.set(data);
  this->pending_ = std::move(state);
}

void Buffer::commit() {
  if (!this->pending_ || !this->pending_->data.get()) {
    return;
  }
  const std::lock_guard lock(this->mutex_);
  this->committed_ = std::move(this->pending_);
}

GLuint Buffer::prepare() {
  if (this->buffer_ == 0) {
    glGenBuffers(1, &this->buffer_);
  }
  std::shared_ptr<State> state;
  {
    const std::lock_guard lock(this->mutex_);
    state.swap(this->committed_);
  }
  if (!state) {
    return this->buffer_;
  }
  {
    const auto data_lock = state->data.lock();
    if (auto* buffer = shm::get_buffer(state->data.get())) {
      glBindBuffer(state->target, this->buffer_);
      auto* data = shm::get_buffer_data(buffer);
      auto  size = shm::get_buffer_size(buffer);
      shm::buffer_begin_access(buffer);
      glBufferData(state->target, size, data, state->usage);
      shm::buffer_end_access(buffer);
    } else if (state->data.get()) {
      LOG_ERR("zwn_buffer: Failed to get shm_buffer from zwn_buffer");
    }
  }
  core::server::get().enqueue_wayland_thread_task([state = std::move(state)] {
    // state.use_count become zero in Wayland thread
  });
  return this->buffer_;
}
void Buffer::bind(GLenum target) {
  glBindBuffer(target, this->prepare());
}

namespace {
constexpr struct zwn_gl_buffer_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .data =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t target,
            wl_resource* data, uint32_t usage) {
          const auto self = core::server::get().resources.get<Buffer>(resource);
          if (self) {
            self->set_data(target, data, usage);
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_buffer(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_buffer_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<Buffer>();
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::gles_v32
