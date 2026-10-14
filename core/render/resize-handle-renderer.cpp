#include "resize-handle-renderer.hpp"

#include <GLES3/gl32.h>

#include <array>
#include <cstddef>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

#include "core/server.hpp"
#include "shader.hpp"

namespace yabe::core::render {
namespace {
// clang-format off
const char* vert_shader = GLSL(
  layout(location = 0) in vec3 pos;

  uniform mat4 mvp_mat;

  out vec2  frag_pos;

  void main() {
    gl_Position = mvp_mat * vec4(pos, 1.0);
    frag_pos = pos.xy;
  }
);
const char* frag_shader = GLSL(
  precision highp float;

  uniform vec2  intersected_pos;
  uniform float alpha;

  in  vec2 frag_pos;
  out vec4 frag_color;

  float lorentz(float x) {
    return 1.0 / (1.0 + x * x);
  }

  void main() {
    vec3 color = vec3(0.9, 0.9, 0.9);
    const float opaque_handle_length = 0.15;
    float d = distance(intersected_pos, frag_pos);
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

void ResizeHandleRenderer::render(
    const glm::mat4& mvp_mat, const glm::vec2& intersected_pos, float alpha) {
  if (!this->initialized_) {
    this->init();
  }

  float original_line_width = 1.F;
  glGetFloatv(GL_LINE_WIDTH, &original_line_width);
  const auto line_width =
      server::get().setting.config.get<"window", "resize_handle_thickness">();
  glLineWidth(static_cast<float>(line_width));

  glUseProgram(this->program_);
  glBindVertexArray(this->vao_);
  glUniform1f(this->alpha_location_, alpha);
  glUniform2f(
      this->intersected_pos_location_, intersected_pos.x, intersected_pos.y);
  glUniformMatrix4fv(
      this->mvp_mat_location_, 1, GL_FALSE, glm::value_ptr(mvp_mat));
  glDrawArrays(GL_LINE_LOOP, 0, 4);
  glBindVertexArray(0);
  glUseProgram(0);
  glLineWidth(original_line_width);
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

  this->mvp_mat_location_ = glGetUniformLocation(this->program_, "mvp_mat");
  this->alpha_location_   = glGetUniformLocation(this->program_, "alpha");
  this->intersected_pos_location_ =
      glGetUniformLocation(this->program_, "intersected_pos");

  glGenVertexArrays(1, &this->vao_);
  glGenBuffers(1, &this->vbo_);

  constexpr std::array<GLfloat, static_cast<std::size_t>(3 * 4)> kVertices = {
      -0.5F, -0.5F, 0.0F,  // Bottom-Left
      +0.5F, -0.5F, 0.0F,  // Bottom-Right
      +0.5F, +0.5F, 0.0F,  // Top-Right
      -0.5F, +0.5F, 0.0F   // Top-Left
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
