#include "sampler.hpp"

#include <GLES3/gl32.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <cstring>
#include <memory>

#include "core/server.hpp"

namespace yabe::proto::zwin::gles_v32 {
Sampler::Sampler() {
  this->pending_.tasks_on_rendering_thread.emplace_back([this] {
    glGenSamplers(1, &this->sampler_);
  });
}

void Sampler::commit() {
  const std::lock_guard lock(this->mutex_);
  this->committed_.tasks_on_rendering_thread.insert(
      this->committed_.tasks_on_rendering_thread.end(),
      std::make_move_iterator(this->pending_.tasks_on_rendering_thread.begin()),
      std::make_move_iterator(this->pending_.tasks_on_rendering_thread.end()));
  this->pending_.tasks_on_rendering_thread.clear();
}
void Sampler::bind(GLuint unit) {
  std::vector<std::function<void()>> tasks;
  {
    const std::lock_guard lock(this->mutex_);
    tasks.swap(this->committed_.tasks_on_rendering_thread);
  }
  for (auto& task : tasks) {
    task();
  }
  glBindSampler(unit, this->sampler_);
}

void Sampler::enqueue_parameter_task(uint32_t pname, GLfloat param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back([this, pname, param] {
    glSamplerParameterf(this->sampler_, pname, param);
  });
}
void Sampler::enqueue_parameter_task(uint32_t pname, GLint param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back([this, pname, param] {
    glSamplerParameteri(this->sampler_, pname, param);
  });
}
void Sampler::enqueue_parameter_task(
    uint32_t pname, std::vector<GLfloat>&& param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back(
      [this, pname, param = std::move(param)] {
        glSamplerParameterfv(this->sampler_, pname, param.data());
      });
}
void Sampler::enqueue_parameter_task(
    uint32_t pname, std::vector<GLint>&& param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back(
      [this, pname, param = std::move(param)] {
        glSamplerParameteriv(this->sampler_, pname, param.data());
      });
}
void Sampler::enqueue_parameter_task_int(
    uint32_t pname, std::vector<GLint>&& param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back(
      [this, pname, param = std::move(param)] {
        glSamplerParameteriv(this->sampler_, pname, param.data());
      });
}
void Sampler::enqueue_parameter_task_int(
    uint32_t pname, std::vector<GLuint>&& param) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.tasks_on_rendering_thread.emplace_back(
      [this, pname, param = std::move(param)] {
        glSamplerParameterIuiv(this->sampler_, pname, param.data());
      });
}

namespace {
template <typename T>
std::vector<T> vector_from_wl_array(wl_array* array) {
  std::vector<T> v(array->size / sizeof(T));
  std::memcpy(v.data(), array->data, array->size);
  return v;
}

constexpr struct zwn_gl_sampler_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .parameterf =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            wl_array* param) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task(
                pname, *static_cast<GLfloat*>(param->data));
          }
        },
    .parameteri =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            int32_t param) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task(pname, param);
          }
        },
    .parameterfv =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            wl_array* params) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task(
                pname, vector_from_wl_array<GLfloat>(params));
          }
        },
    .parameteriv =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            wl_array* params) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task(
                pname, vector_from_wl_array<GLint>(params));
          }
        },
    .parameterIiv =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            wl_array* params) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task_int(
                pname, vector_from_wl_array<GLint>(params));
          }
        },
    .parameterIuiv =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t pname,
            wl_array* params) {
          const auto self =
              core::server::get().resources.get<Sampler>(resource);
          if (self) {
            self->enqueue_parameter_task_int(
                pname, vector_from_wl_array<GLuint>(params));
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_sampler(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_sampler_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<Sampler>();
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::gles_v32
