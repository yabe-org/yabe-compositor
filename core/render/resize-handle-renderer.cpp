#include "resize-handle-renderer.hpp"

#include <GLES3/gl32.h>

#include <array>
#include <cstddef>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

#include "common/unit.hpp"
#include "core/server.hpp"
#include "shader.hpp"

namespace yabe::core::render {
namespace {
// clang-format off
const char* vert_shader = GLSL(
  layout(location = 0) in vec3 vert;

  uniform mat4 vp_mat;
  uniform mat4 world_transform;
  uniform vec2 half_size;
  uniform float thickness;

  out vec4  frag_pos;

  void main() {
    vec2 pos = vert.xy * (half_size + (thickness * vert.z));
    frag_pos = world_transform * vec4(pos, 0.0, 1.0);
    gl_Position = vp_mat * frag_pos;
  }
);
const char* frag_shader = GLSL(
  precision highp float;

  uniform vec3  intersected_pos;
  uniform float alpha;

  in  vec4 frag_pos;
  out vec4 frag_color;

  float lorentz(float x) {
    return 1.0 / (1.0 + x * x);
  }

  // TODO: make color, length, multiplier (64.0) configurable
  void main() {
    vec3 color = vec3(0.9, 0.9, 0.9);
    const float opaque_handle_length = 0.04;
    float d = distance(intersected_pos, frag_pos.xyz);
    // if distance < opaque_handle_length then alpha = 1.0; else alpha = lorentz_func
    float actual_alpha = mix(
        1.0,
        lorentz(64.0 * (d - opaque_handle_length)),
        step(opaque_handle_length, d)
    );
    // lorentz func does not return zero, so clamp it if the result is enough small
    actual_alpha = mix(0.0, actual_alpha * alpha, step(0.01, actual_alpha));
    frag_color = vec4(color * actual_alpha, actual_alpha);
  }
);
// clang-format on
}  // namespace

ResizeHandleRenderer::~ResizeHandleRenderer() {
  if (!this->initialized_) {
    return;
  }
  glDeleteProgram(this->program_);
  glDeleteShader(this->vert_shader_);
  glDeleteShader(this->frag_shader_);
  glDeleteVertexArrays(1, &this->vao_);
  glDeleteBuffers(1, &this->vbo_);
}

void ResizeHandleRenderer::render(const glm::mat4& vp_mat,
    const glm::mat4& world_transform, const glm::vec2& half_size,
    const glm::vec3& intersected_pos, float alpha) {
  if (!this->initialized_) {
    this->init();
  }

  const common::unit::Pixel thickness{
      server::get().setting.config.get<"window", "resize_handle_thickness">()};
  glUseProgram(this->program_);
  glBindVertexArray(this->vao_);
  glUniform1f(this->alpha_location_, alpha);
  glUniform1f(this->thickness_location_, *thickness.to_world_unit());
  glUniform2fv(this->half_size_location_, 1, glm::value_ptr(half_size));
  glUniform3fv(
      this->intersected_pos_location_, 1, glm::value_ptr(intersected_pos));
  glUniformMatrix4fv(
      this->vp_mat_location_, 1, GL_FALSE, glm::value_ptr(vp_mat));
  glUniformMatrix4fv(this->world_transform_location_, 1, GL_FALSE,
      glm::value_ptr(world_transform));
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);
  glBindVertexArray(0);
  glUseProgram(0);
}

void ResizeHandleRenderer::invalidate() {
  this->initialized_ = false;
}

void ResizeHandleRenderer::init() {
  // If compilation fails, the compositor won't work,
  // so use `.value()` so that it throws an exception
  this->vert_shader_ = compile_glsl(GL_VERTEX_SHADER, vert_shader).value();
  this->frag_shader_ = compile_glsl(GL_FRAGMENT_SHADER, frag_shader).value();
  this->program_     = glCreateProgram();
  glAttachShader(this->program_, this->vert_shader_);
  glAttachShader(this->program_, this->frag_shader_);
  if (!link_program(this->program_)) {
    throw std::runtime_error(
        "Failed to link ResizeHandleRenderer shader program");
  }

  this->vp_mat_location_ = glGetUniformLocation(this->program_, "vp_mat");
  this->world_transform_location_ =
      glGetUniformLocation(this->program_, "world_transform");
  this->half_size_location_ = glGetUniformLocation(this->program_, "half_size");
  this->thickness_location_ = glGetUniformLocation(this->program_, "thickness");
  this->alpha_location_     = glGetUniformLocation(this->program_, "alpha");
  this->intersected_pos_location_ =
      glGetUniformLocation(this->program_, "intersected_pos");

  glGenVertexArrays(1, &this->vao_);
  glGenBuffers(1, &this->vbo_);

  constexpr std::array<GLfloat, static_cast<std::size_t>(3 * 10)> kVertices = {
      // inner       , outer
      -1.F, -1.F, 0.F, -1.F, -1.F, 1.F,  // Bottom-Left
      +1.F, -1.F, 0.F, +1.F, -1.F, 1.F,  // Bottom-Right
      +1.F, +1.F, 0.F, +1.F, +1.F, 1.F,  // Top-Right
      -1.F, +1.F, 0.F, -1.F, +1.F, 1.F,  // Top-Left
      -1.F, -1.F, 0.F, -1.F, -1.F, 1.F   // Bottom-Left (close)
  };
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * kVertices.size(),
      kVertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  this->initialized_ = true;
}
}  // namespace yabe::core::render
