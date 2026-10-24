#include "ray-renderer.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

#include "shader.hpp"

namespace yabe::core::render {
namespace {
// clang-format off
const char* vert_shader = GLSL(
  layout(location = 0) in float t;

  uniform mat4 vp_mat;
  uniform vec3 start_pos;
  uniform vec3 end_pos;
  out vec3 pos;
  out float ray_t;

  void main() {
    pos = mix(start_pos, end_pos, t);
    ray_t = t;
    gl_Position = vp_mat * vec4(pos, 1.0);
  }
);
const char* frag_shader = GLSL(
  precision highp float;
  in  vec3 pos;
  in  float ray_t;
  out vec4 frag_color;

  void main() {
    // opaque until t < edge
    float edge = 0.40;
    float alpha = mix(1.0, 1.0 - ((ray_t - edge) * (1.0 / edge))*0.95, step(edge, ray_t));
    vec3 color = vec3(0.11, 0.93, 0.07 + (0.79 * (ray_t + 0.24)));
    frag_color = vec4(color * alpha, alpha);
  }
);
// clang-format on
}  // namespace

RayRenderer::~RayRenderer() {
  if (!this->initialized_) {
    return;
  }
  glDeleteProgram(this->program_);
  glDeleteShader(this->vert_shader_);
  glDeleteShader(this->frag_shader_);
  glDeleteVertexArrays(1, &this->vao_);
  glDeleteBuffers(1, &this->vbo_);
}

void RayRenderer::render(const glm::mat4& vp_mat, const glm::vec3& start_pos,
    const glm::vec3& direction, float length) {
  if (!this->initialized_) {
    this->init();
  }

  glUseProgram(this->program_);
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glUniformMatrix4fv(
      this->vp_mat_location_, 1, GL_FALSE, glm::value_ptr(vp_mat));

  glm::vec3 end_pos = start_pos + (direction * length);

  glUniform3fv(this->start_pos_location_, 1, glm::value_ptr(start_pos));
  glUniform3fv(this->end_pos_location_, 1, glm::value_ptr(end_pos));

  glDrawArrays(GL_LINES, 0, 2);
}

void RayRenderer::invalidate() {
  this->initialized_ = false;
}

void RayRenderer::init() {
  this->vert_shader_ = compile_glsl(GL_VERTEX_SHADER, vert_shader).value();
  this->frag_shader_ = compile_glsl(GL_FRAGMENT_SHADER, frag_shader).value();
  this->program_     = glCreateProgram();
  glAttachShader(this->program_, this->vert_shader_);
  glAttachShader(this->program_, this->frag_shader_);
  if (!link_program(this->program_)) {
    throw std::runtime_error("Failed to link RayRenderer shader program");
  }

  this->vp_mat_location_    = glGetUniformLocation(this->program_, "vp_mat");
  this->start_pos_location_ = glGetUniformLocation(this->program_, "start_pos");
  this->end_pos_location_   = glGetUniformLocation(this->program_, "end_pos");

  glGenVertexArrays(1, &this->vao_);
  glGenBuffers(1, &this->vbo_);
  const std::array<GLfloat, 2> vertices = {0.F, 1.F};
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(),
      vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  this->initialized_ = true;
}
}  // namespace yabe::core::render
