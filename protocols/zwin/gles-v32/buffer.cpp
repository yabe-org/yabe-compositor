#include "buffer.hpp"

#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/server.hpp"
#include "protocols/zwin/shm/shm-buffer.hpp"
#include "wayland-server-core.h"

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
  const std::lock_guard lock(this->mutex_);
  this->pending_.target = target;
  this->pending_.usage  = usage;
  this->pending_.data.set(data);
}

void Buffer::commit() {
  if (!this->pending_.data.get()) {
    return;
  }
  const std::lock_guard lock(this->mutex_);
  this->committed_.target = this->pending_.target;
  this->committed_.usage  = this->pending_.usage;
  this->committed_.data.set(this->pending_.data.get());
  this->pending_.reset();
}

GLuint Buffer::prepare() {
  if (this->buffer_ == 0) {
    glGenBuffers(1, &this->buffer_);
  }
  if (!this->committed_.data.get()) {
    return this->buffer_;
  }
  const std::lock_guard lock(this->mutex_);
  glBindBuffer(this->committed_.target, this->buffer_);
  auto* buffer = shm::get_buffer(this->committed_.data.get());
  if (!buffer) {
    LOG_ERR("zwn_buffer: Failed to get shm_buffer from zwn_buffer");
    return this->buffer_;
  }
  auto* data = shm::get_buffer_data(buffer);
  auto  size = shm::get_buffer_size(buffer);
  shm::buffer_begin_access(buffer);
  glBufferData(this->committed_.target, size, data, this->committed_.usage);
  shm::buffer_end_access(buffer);
  this->committed_.reset();
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
