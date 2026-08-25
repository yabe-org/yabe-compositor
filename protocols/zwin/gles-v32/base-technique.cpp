#include "base-technique.hpp"

#include <GLES3/gl32.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <cstring>
#include <memory>
#include <utility>
#include <variant>

#include "buffer.hpp"
#include "common/convert-wl-array.hpp"
#include "common/visitor-list.hpp"
#include "core/server.hpp"
#include "program.hpp"
#include "rendering-unit.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "vertex-array.hpp"

namespace yabe::proto::zwin::gles_v32 {
namespace {
// std::function cannot be used in constexpr context
using UniformVectorFunction = void (*)(GLint, GLsizei, void*);
// clang-format off
// NOLINTNEXTLINE: std::array cannot be used in constexpr context/
constexpr UniformVectorFunction kApplyUniformVectorFunctions[/*type*/][4/*size*/] = {
  { // ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_INT
    [](GLint location, GLsizei count, void* value) {
      glUniform1iv(location, count, static_cast<GLint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform2iv(location, count, static_cast<GLint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform3iv(location, count, static_cast<GLint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform4iv(location, count, static_cast<GLint*>(value));
    }, 
  },
  { // ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_UINT
    [](GLint location, GLsizei count, void* value) {
      glUniform1uiv(location, count, static_cast<GLuint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform2uiv(location, count, static_cast<GLuint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform3uiv(location, count, static_cast<GLuint*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform4uiv(location, count, static_cast<GLuint*>(value));
    },
  },
  { // ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_FLOAT
    [](GLint location, GLsizei count, void* value) {
      glUniform1fv(location, count, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform2fv(location, count, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform3fv(location, count, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, void* value) {
      glUniform4fv(location, count, static_cast<GLfloat*>(value));
    },
  }
};
// clang-format on

using UniformMatrixFunction = void (*)(GLint, GLsizei, GLboolean, void*);
// clang-format off
// NOLINTNEXTLINE: std::array cannot be used in constexpr context
constexpr UniformMatrixFunction kApplyUniformMatrixFunctions[/*col*/][3 /*row*/] = {
  { // col = 2
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix2fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix2x3fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix2x4fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
  },
  { // col = 3
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix3x2fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix3fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix3x4fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
  },
  { // col = 4
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix4x2fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix4x3fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
    [](GLint location, GLsizei count, GLboolean transpose, void* value) {
      glUniformMatrix4fv(
          location, count, transpose, static_cast<GLfloat*>(value));
    },
  },
};
// clang-format on
}  // namespace

void BaseTechnique::bind_program(const std::shared_ptr<Program>& program) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.program = program;
}

void BaseTechnique::bind_vertex_array(
    const std::shared_ptr<VertexArray>& vertex_array) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.vertex_array = vertex_array;
}

void BaseTechnique::bind_texture(GLuint binding, std::string name,
    const std::shared_ptr<Texture>& texture, GLenum target,
    const std::shared_ptr<Sampler>& sampler) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.textures.insert_or_assign(
      binding, TextureBinding{.name = std::move(name),
                   .texture         = texture,
                   .target          = target,
                   .sampler         = sampler});
}

void BaseTechnique::set_uniform_vector(GLuint location, std::string name,
    uint32_t type, uint32_t size, GLsizei count, std::vector<std::byte> value) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.uniforms.emplace_back(UniformVector{
      .location = static_cast<GLint>(location),
      .name     = std::move(name),
      .type     = type,
      .size     = size,
      .count    = count,
      .value    = std::move(value),
  });
}

void BaseTechnique::set_uniform_matrix(GLuint location, std::string name,
    uint32_t col, uint32_t row, GLsizei count, GLboolean transpose,
    std::vector<std::byte> value) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.uniforms.emplace_back(UniformMatrix{
      .location  = static_cast<GLint>(location),
      .name      = std::move(name),
      .col       = col,
      .row       = row,
      .count     = count,
      .transpose = transpose,
      .value     = std::move(value),
  });
}

void BaseTechnique::draw_arrays(GLenum mode, GLint first, GLsizei count) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.draws.emplace_back(
      DrawArrays{.mode = mode, .first = first, .count = count});
}

void BaseTechnique::draw_elements(GLenum mode, GLsizei count, GLenum type,
    off_t offset, const std::shared_ptr<Buffer>& buffer) {
  const std::lock_guard lock(this->mutex_);
  this->pending_.draws.emplace_back(DrawElements{
      .mode   = mode,
      .count  = count,
      .type   = type,
      .offset = offset,
      .buffer = buffer,
  });
}

void BaseTechnique::commit() {
  {
    const std::lock_guard lock(this->mutex_);
    if (this->pending_.program.lock()) {
      this->current_.program = this->pending_.program;
    }
    if (this->pending_.vertex_array.lock()) {
      this->current_.vertex_array = this->pending_.vertex_array;
    }
    for (auto& [binding, texture_binding] : this->pending_.textures) {
      this->current_.textures.insert_or_assign(
          binding, std::move(texture_binding));
    }
    this->current_.uniforms.insert(this->current_.uniforms.end(),
        std::make_move_iterator(this->pending_.uniforms.begin()),
        std::make_move_iterator(this->pending_.uniforms.end()));
    this->current_.draws.insert(this->current_.draws.end(),
        std::make_move_iterator(this->pending_.draws.begin()),
        std::make_move_iterator(this->pending_.draws.end()));
    this->pending_.reset();
  }
  if (const auto program = this->current_.program.lock()) {
    program->commit();
  }
  if (const auto vertex_array = this->current_.vertex_array.lock()) {
    vertex_array->commit();
  }
  for (const auto& [_, binding] : this->current_.textures) {
    if (const auto texture = binding.texture.lock()) {
      texture->commit();
    }
    if (const auto sampler = binding.sampler.lock()) {
      sampler->commit();
    }
  }
  for (const auto& draw : this->current_.draws) {
    const auto* elements = std::get_if<DrawElements>(&draw);
    if (!elements) {
      continue;
    }
    if (const auto buffer = elements->buffer.lock()) {
      buffer->commit();
    }
  }
}

void BaseTechnique::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
    const glm::mat4& model_mat) {
  const std::lock_guard lock(this->mutex_);
  auto                  program      = this->current_.program.lock();
  auto                  vertex_array = this->current_.vertex_array.lock();
  if (!program || !vertex_array) {
    return;
  }
  program->use(proj_mat, view_mat, model_mat);
  vertex_array->bind();
  for (const auto& [binding, texture_binding] : this->current_.textures) {
    glActiveTexture(GL_TEXTURE0 + binding);
    auto texture = texture_binding.texture.lock();
    auto sampler = texture_binding.sampler.lock();
    if (!texture || !sampler) {
      continue;
    }
    sampler->bind(binding);
    texture->bind(texture_binding.target);
    if (!texture_binding.name.empty()) {
      const auto location = program->get_location(texture_binding.name);
      glUniform1i(location, static_cast<GLint>(binding));
    }
  }
  for (auto& uniform : this->current_.uniforms) {
    common::VisitorList(
        [&program](UniformVector& uniform) {
          if (uniform.location == 0) {
            uniform.location = program->get_location(uniform.name);
          }
          kApplyUniformVectorFunctions[uniform.type][uniform.size - 1](
              uniform.location, uniform.count, uniform.value.data());
        },
        [&program](UniformMatrix& uniform) {
          if (uniform.location == 0) {
            uniform.location = program->get_location(uniform.name);
          }
          kApplyUniformMatrixFunctions[uniform.col - 2][uniform.row - 2](
              uniform.location, uniform.count, uniform.transpose,
              uniform.value.data());
        })
        .visit(uniform);
  }
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  for (auto& draw : this->current_.draws) {
    common::VisitorList(
        [](DrawArrays& arrays) {
          glDrawArrays(arrays.mode, arrays.first, arrays.count);
        },
        [](DrawElements& elements) {
          auto buffer = elements.buffer.lock();
          if (!buffer) {
            return;
          }
          buffer->bind(GL_ELEMENT_ARRAY_BUFFER);
          glDrawElements(elements.mode, elements.count, elements.type,
              reinterpret_cast<const void*>(elements.offset));  // NOLINT
        })
        .visit(draw);
  }
  glDisable(GL_DEPTH_TEST);
  glBindVertexArray(0);
  glUseProgram(0);
  for (const auto& [binding, texture_binding] : this->current_.textures) {
    glBindSampler(binding, 0);
    glBindTexture(texture_binding.target, 0);
  }
}

namespace {
std::vector<std::byte> bytes_from_wl_array(wl_array* value) {
  if (value == nullptr) {
    return {};
  }
  std::vector<std::byte> bytes(value->size);
  std::memcpy(bytes.data(), value->data, value->size);
  return bytes;
}

constexpr struct zwn_gl_base_technique_interface kImpl = {
    .destroy =
        [](wl_client* /*client*/, wl_resource* resource) {
          wl_resource_destroy(resource);
        },
    .bind_program =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* program_resource) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const auto program =
              core::server::get().resources.get<Program>(program_resource);
          if (self && program) {
            self->bind_program(program);
          }
        },
    .bind_vertex_array =
        [](wl_client* /*client*/, wl_resource* resource,
            wl_resource* vertex_array_resource) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const auto vertex_array =
              core::server::get().resources.get<VertexArray>(
                  vertex_array_resource);
          if (self && vertex_array) {
            self->bind_vertex_array(vertex_array);
          }
        },
    .bind_texture =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t binding,
            const char* name, wl_resource* texture_resource, uint32_t target,
            wl_resource* sampler_resource) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const auto texture =
              core::server::get().resources.get<Texture>(texture_resource);
          const auto sampler =
              core::server::get().resources.get<Sampler>(sampler_resource);
          if (self && texture && sampler) {
            self->bind_texture(binding, name, texture, target, sampler);
          }
        },
    .uniform_vector =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t location,
            const char* name, uint32_t type, uint32_t size, uint32_t count,
            wl_array* value) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const bool valid_type =
              type == ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_INT ||
              type == ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_UINT ||
              type == ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_FLOAT;
          const std::size_t expected =
              sizeof(uint32_t) * static_cast<std::size_t>(size) * count;
          if (!self || !valid_type || size < 1 || size > 4 ||
              value == nullptr || value->size != expected) {
            wl_resource_post_error(resource,
                ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
                "invalid zwn_gl_base_technique uniform vector");
            return;
          }
          self->set_uniform_vector(location, name, type, size,
              static_cast<GLsizei>(count), bytes_from_wl_array(value));
        },
    .uniform_matrix =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t location,
            const char* name, uint32_t col, uint32_t row, uint32_t count,
            uint32_t transpose, wl_array* value) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const std::size_t expected =
              sizeof(GLfloat) * static_cast<std::size_t>(col) * row * count;
          if (!self || col < 2 || col > 4 || row < 2 || row > 4 ||
              value == nullptr || value->size != expected) {
            wl_resource_post_error(resource,
                ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
                "invalid zwn_gl_base_technique uniform matrix");
            return;
          }
          self->set_uniform_matrix(location, name, col, row,
              static_cast<GLsizei>(count), transpose == 0 ? GL_FALSE : GL_TRUE,
              bytes_from_wl_array(value));
        },
    .draw_arrays =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t mode,
            int32_t first, uint32_t count) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          if (self) {
            self->draw_arrays(mode, first, static_cast<GLsizei>(count));
          }
        },
    .draw_elements =
        [](wl_client* /*client*/, wl_resource* resource, uint32_t mode,
            uint32_t count, uint32_t type, wl_array* offset_array,
            wl_resource* buffer_resource) {
          const auto self =
              core::server::get().resources.get<BaseTechnique>(resource);
          const auto buffer =
              core::server::get().resources.get<Buffer>(buffer_resource);
          off_t offset = 0;
          if (self && buffer && common::from_wl_array(offset_array, &offset)) {
            self->draw_elements(
                mode, static_cast<GLsizei>(count), type, offset, buffer);
          }
        },
};

void handle_destruction(wl_resource* resource) {
  core::server::get().resources.remove(resource);
}
}  // namespace

void create_base_technique(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<RenderingUnit>& rendering_unit) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_base_technique_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, handle_destruction);
  const auto self = std::make_shared<BaseTechnique>();
  core::server::get().resources.add(resource, self);
  if (!rendering_unit->set_technique(self)) {
    wl_resource_post_error(resource, ZWN_GL_BASE_TECHNIQUE_ERROR_TECHNIQUE,
        "zwn_rendering_unit already has a technique");
  }
}
}  // namespace yabe::proto::zwin::gles_v32
