#include "vertex-array.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>
#include <utility>

#include "buffer.hpp"
#include "common/convert-wl-array.hpp"
#include "core/server.hpp"

namespace yabe::proto::zwin::gles_v32 {
VertexArray::~VertexArray() {
  if (this->vertex_array_ == 0) {
    return;
  }
  const GLuint vertex_array = this->vertex_array_;
  core::server::get().enqueue_rendering_thread_task([vertex_array] {
    glDeleteVertexArrays(1, &vertex_array);
  });
}

void VertexArray::set_enabled(GLuint index, bool enabled) {
  this->pending_[index].enabled = enabled;
}

void VertexArray::set_attribute(GLuint index, GLint size, GLenum type,
    GLboolean normalized, GLsizei stride, off_t offset,
    const std::shared_ptr<Buffer>& buffer) {
  const std::lock_guard lock(this->mutex_);
  bool                  enabled = false;
  auto                  it      = this->pending_.find(index);
  if (it != this->pending_.end()) {
    enabled = it->second.enabled;
  }
  this->pending_[index] = Attribute{
      .enabled    = enabled,
      .size       = size,
      .type       = type,
      .normalized = normalized,
      .stride     = stride,
      .offset     = offset,
      .buffer     = buffer,
  };
}

void VertexArray::commit() {
  {
    const std::lock_guard lock(this->mutex_);
    if (this->pending_.empty()) {
      return;
    }
    this->committed_ = std::move(this->pending_);
    this->pending_.clear();
  }
  for (const auto& [index, attribute] : this->committed_) {
    if (const auto buffer = attribute.buffer.lock()) {
      buffer->commit();
    }
  }
}

void VertexArray::bind() {
  if (this->vertex_array_ == 0) {
    glGenVertexArrays(1, &this->vertex_array_);
  }
  glBindVertexArray(this->vertex_array_);
  if (this->committed_.empty()) {
    return;
  }
  State state;
  {
    const std::lock_guard lock(this->mutex_);
    state.swap(this->committed_);
  }
  for (const auto& [index, attribute] : state) {
    const auto buffer = attribute.buffer.lock();
    if (!buffer) {
      continue;
    }
    buffer->bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(index, attribute.size, attribute.type,
        attribute.normalized, attribute.stride,
        // NOLINTNEXTLINE(google-readability-casting, performance-no-int-to-ptr)
        (const void*)attribute.offset);
    if (attribute.enabled) {
      glEnableVertexAttribArray(index);
    } else {
      glDisableVertexAttribArray(index);
    }
    current_.insert_or_assign(index, attribute);
  }
}

namespace {
constexpr struct zwn_gl_vertex_array_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .enable_vertex_attrib_array =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t index) {
          const auto self =
              core::server::get().resources.get<VertexArray>(resource);
          if (self) {
            self->set_enabled(index, true);
          }
        },
    .disable_vertex_attrib_array =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t index) {
          const auto self =
              core::server::get().resources.get<VertexArray>(resource);
          if (self) {
            self->set_enabled(index, false);
          }
        },
    .vertex_attrib_pointer =
        [](wl_client* client, wl_resource* resource, uint32_t index,
            int32_t size, uint32_t type, uint32_t normalized, int32_t stride,
            wl_array* offset_array, wl_resource* buffer_resource) {
          off_t offset = 0;
          if (!common::from_wl_array(offset_array, &offset)) {
            wl_client_post_implementation_error(client,
                "zwn_gl_vertex_array.vertex_attrib_pointer: invalid wl_array");
            return;
          }
          const auto self =
              core::server::get().resources.get<VertexArray>(resource);
          const auto buffer =
              core::server::get().resources.get<Buffer>(buffer_resource);
          if (!self || !buffer) {
            return;
          }
          self->set_attribute(index, size, type,
              normalized == 0 ? GL_FALSE : GL_TRUE, stride, offset, buffer);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_vertex_array(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_vertex_array_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  core::server::get().resources.add(resource, std::make_shared<VertexArray>());
}
}  // namespace yabe::proto::zwin::gles_v32
