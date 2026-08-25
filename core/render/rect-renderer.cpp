#include "rect-renderer.hpp"

#include <GLES3/gl32.h>

#include <array>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

#include "shader.hpp"

namespace yabe::core::render {
namespace {
// clang-format off
const char* vert_shader = GLSL(
  layout(location = 0) in vec3 pos;
  layout(location = 1) in vec2 tex_coord;
  out vec2 tex_coord_out;

  uniform mat4 mvp_mat;

  void main() {
    gl_Position = mvp_mat * vec4(pos, 1.0);
    tex_coord_out = tex_coord;
  }
);
const char* frag_shader = GLSL(
  precision mediump float;
  uniform sampler2D texture_sampler;

  in  vec2 tex_coord_out;
  out vec4 frag_color;

  void main() {
    frag_color = texture(texture_sampler, tex_coord_out);
  }
);
// clang-format on
}  // namespace

RectRenderer::~RectRenderer() {
  if (!this->initialized_) {
    return;
  }
  glDeleteProgram(this->program_);
  glDeleteShader(this->vert_shader_);
  glDeleteShader(this->frag_shader_);
  glDeleteVertexArrays(1, &this->vao_);
  glDeleteBuffers(1, &this->vbo_);
}

void RectRenderer::render(const glm::mat4& mvp_mat, const Texture& texture) {
  if (!this->initialized_) {
    this->init();
  }

  glUseProgram(this->program_);
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(this->texture_sampler_location_, 0);
  glUniformMatrix4fv(
      this->mvp_mat_location_, 1, GL_FALSE, glm::value_ptr(mvp_mat));
  glBindTexture(GL_TEXTURE_2D, texture.get_texture_id());
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}

void RectRenderer::init() {
  // If compilation fails, the compositor won't work,
  // so use `.value()` so that it throws an exception
  this->vert_shader_ = compile_glsl(GL_VERTEX_SHADER, vert_shader).value();
  this->frag_shader_ = compile_glsl(GL_FRAGMENT_SHADER, frag_shader).value();
  this->program_     = glCreateProgram();
  glAttachShader(this->program_, this->vert_shader_);
  glAttachShader(this->program_, this->frag_shader_);
  if (!link_program(this->program_)) {
    throw std::runtime_error("Failed to link RectRenderer shader program");
  }

  this->mvp_mat_location_ = glGetUniformLocation(this->program_, "mvp_mat");
  this->texture_sampler_location_ =
      glGetUniformLocation(this->program_, "texture_sampler");

  glGenVertexArrays(1, &this->vao_);
  glGenBuffers(1, &this->vbo_);

  constexpr std::array<GLfloat, 5 * 4> kVertices = {
      // Positions x 3, UV x 2
      -0.5F, -0.5F, 0.0F, 0.0F, 1.0F,  // Bottom-Left
      +0.5F, -0.5F, 0.0F, 1.0F, 1.0F,  // Bottom-Right
      +0.5F, +0.5F, 0.0F, 1.0F, 0.0F,  // Top-Right
      -0.5F, +0.5F, 0.0F, 0.0F, 0.0F   // Top-Left
  };
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * kVertices.size(),
      kVertices.data(), GL_STATIC_DRAW);

  // 0: position
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);
  // 1: texture coordinate
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
      (GLvoid*)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  this->initialized_ = true;
}
}  // namespace yabe::core::render
