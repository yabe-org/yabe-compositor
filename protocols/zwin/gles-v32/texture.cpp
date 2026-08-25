#include "texture.hpp"

#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <functional>
#include <iterator>
#include <limits>
#include <memory>

#include "common/weak-resource.hpp"
#include "core/server.hpp"
#include "protocols/zwin/shm/shm-buffer.hpp"
#include "wayland-server-protocol.h"

namespace yabe::proto::zwin::gles_v32 {
Texture::~Texture() {
  if (this->texture_ == 0) {
    return;
  }
  const GLuint texture = this->texture_;
  core::server::get().enqueue_rendering_thread_task([texture] {
    glDeleteTextures(1, &texture);
  });
}

void Texture::image_2d(GLenum target, GLint level, GLint internal_format,
    GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
    wl_resource* zwn_buffer_resource) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.emplace_back(
      [target, level, internal_format, width, height, border, format, type,
          data_resource = common::WeakResource(zwn_buffer_resource)] {
        if (!data_resource.get()) {
          return;
        }
        auto* buffer = shm::get_buffer(data_resource.get());
        if (!buffer) {
          LOG_ERR("zwn_texture: Failed to get Buffer from zwn_buffer");
          return;
        }
        auto* data = shm::get_buffer_data(buffer);
        shm::buffer_begin_access(buffer);
        glTexImage2D(target, level, internal_format, width, height, border,
            format, type, data);
        shm::buffer_end_access(buffer);
      });
}

void Texture::generate_mipmap(GLenum target) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.emplace_back([target] {
    glGenerateMipmap(target);
  });
}

void Texture::commit() {
  const std::lock_guard lock(this->mutex_);
  if (this->pending_.empty()) {
    return;
  }
  this->committed_.insert(this->committed_.end(),
      std::make_move_iterator(this->pending_.begin()),
      std::make_move_iterator(this->pending_.end()));
  this->pending_.clear();
}

void Texture::bind(GLenum target) {
  std::vector<std::function<void()>> tasks;
  {
    const std::lock_guard lock(this->mutex_);
    tasks.swap(this->committed_);
  }
  if (this->texture_ == 0) {
    glGenTextures(1, &this->texture_);
  }
  glBindTexture(target, this->texture_);
  for (const auto& task : tasks) {
    task();
  }
}

namespace {
constexpr struct zwn_gl_texture_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .image_2d =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t target,
            int32_t level, int32_t internal_format, uint32_t width,
            uint32_t height, int32_t border, uint32_t format, uint32_t type,
            wl_resource* data) {
          const auto self =
              core::server::get().resources.get<Texture>(resource);
          if (!self) {
            return;
          }
          if (width >
                  static_cast<uint32_t>(std::numeric_limits<GLsizei>::max()) ||
              height >
                  static_cast<uint32_t>(std::numeric_limits<GLsizei>::max())) {
            wl_resource_post_error(resource,
                ZWN_GL_TEXTURE_ERROR_INVALID_IMAGE_SIZE,
                "zwn_gl_texture image dimensions exceed GLsizei");
            return;
          }
          self->image_2d(target, level, internal_format,
              static_cast<GLsizei>(width), static_cast<GLsizei>(height), border,
              format, type, data);
        },
    .generate_mipmap =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t target) {
          const auto self =
              core::server::get().resources.get<Texture>(resource);
          if (self) {
            self->generate_mipmap(target);
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_texture(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_texture_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  core::server::get().resources.add(resource, std::make_shared<Texture>());
}
}  // namespace yabe::proto::zwin::gles_v32
