#include "shader.hpp"

#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>
#include <utility>

#include "core/render/shader.hpp"
#include "core/server.hpp"

namespace yabe::proto::zwin::gles_v32 {
Shader::Shader(std::string&& source, GLenum type)
    : source_(std::move(source)), type_(type) {
}

Shader::~Shader() {
  if (this->shader_ == 0) {
    return;
  }
  const GLuint shader = this->shader_;
  core::server::get().enqueue_rendering_thread_task([shader] {
    glDeleteShader(shader);
  });
}

GLuint Shader::prepare() {
  if (this->is_uploaded_) {
    return this->shader_;
  }
  this->is_uploaded_ = true;
  this->shader_ = core::render::compile_glsl(this->type_, this->source_.c_str())
                      .value_or(0);
  return this->shader_;
}

namespace {
constexpr struct zwn_gl_shader_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_shader(wl_client* client, int version, uint32_t id,
    std::string&& source, uint32_t type) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_shader_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<Shader>(std::move(source), type);
  core::server::get().resources.add(resource, self);
}
}  // namespace yabe::proto::zwin::gles_v32
