#include "program.hpp"

#include <GLES3/gl32.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>

#include "common/common.hpp"
#include "core/server.hpp"
#include "shader.hpp"

namespace yabe::proto::zwin::gles_v32 {
Program::~Program() {
  if (this->program_ == 0) {
    return;
  }
  const GLuint program = this->program_;
  core::server::get().enqueue_rendering_thread_task([program] {
    glDeleteProgram(program);
  });
}

void Program::attach_shader(const std::shared_ptr<Shader>& shader) {
  const std::lock_guard lock(this->mutex_);
  this->shaders_.emplace_back(shader);
}

void Program::request_link() {
  if (this->state_ == State::LINKED) {
    return;
  }
  const std::lock_guard lock(this->mutex_);
  this->state_ = State::LINK_REQUESTED;
}
GLint Program::get_location(const std::string& name) {
  if (this->state_ != State::LINKED) {
    return -1;
  }
  return glGetUniformLocation(this->program_, name.c_str());
}

void Program::commit() {
  if (this->state_ != State::LINK_REQUESTED) {
    return;
  }
  const std::lock_guard lock(this->mutex_);
  this->state_ = State::WAIT_FOR_LINK;
}

void Program::use(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  if (this->state_ == State::WAIT_FOR_LINK) {
    this->program_ = glCreateProgram();
    for (const auto& weak_shader : this->shaders_) {
      if (const auto shader = weak_shader.lock()) {
        glAttachShader(this->program_, shader->prepare());
      }
    }
    glLinkProgram(this->program_);
    GLint linked = GL_FALSE;
    glGetProgramiv(this->program_, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      GLint length = 0;
      glGetProgramiv(this->program_, GL_INFO_LOG_LENGTH, &length);
      std::vector<GLchar> log(static_cast<std::size_t>(length) + 1);
      glGetProgramInfoLog(this->program_, length, nullptr, log.data());
      LOG_ERR("Failed to link zwn_gl_program: {}", log.data());
    }
    this->state_ = State::LINKED;
  }
  glUseProgram(this->program_);
  std::array<std::pair<const char*, const glm::mat4&>, 5> uniform_pairs{
      std::pair{"zProjection", proj_mat                       },
      std::pair{"zView",       view_mat                       },
      std::pair{"zModel",      model_mat                      },
      std::pair{"zVP",         proj_mat * view_mat            },
      std::pair{"zMVP",        proj_mat * view_mat * model_mat},
  };
  for (const auto& [name, value] : uniform_pairs) {
    const auto location = glGetUniformLocation(this->program_, name);
    if (location == -1) {
      continue;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
  }
}

namespace {
constexpr struct zwn_gl_program_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .attach_shader =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* shader_resource) {
          const auto self =
              core::server::get().resources.get<Program>(resource);
          const auto shader =
              core::server::get().resources.get<Shader>(shader_resource);
          if (!self || !shader) {
            return;
          }
          self->attach_shader(shader);
        },
    .link =
        [](wl_client* /*client*/, wl_resource* resource) {
          const auto self =
              core::server::get().resources.get<Program>(resource);
          if (!self) {
            return;
          }
          if (self->is_linked()) {
            wl_resource_post_error(resource, ZWN_GL_PROGRAM_ERROR_RELINK,
                "zwn_gl_program has already been linked");
            return;
          }
          self->request_link();
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_program(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_program_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  core::server::get().resources.add(resource, std::make_shared<Program>());
}
}  // namespace yabe::proto::zwin::gles_v32
