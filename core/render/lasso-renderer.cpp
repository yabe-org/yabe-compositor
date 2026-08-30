#include "lasso-renderer.hpp"

#include <GLES3/gl32.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdexcept>

#include "core/server.hpp"
#include "glm/ext/vector_float3.hpp"
#include "shader.hpp"

namespace yabe::core::render {
namespace {
// clang-format off
const char* vert_shader = GLSL(
  layout(location = 0) in vec3 pos;

  uniform mat4 vp_mat;

  void main() {
    gl_Position = vp_mat * vec4(pos, 1.0);
  }
);
const char* frag_shader = GLSL(
  precision mediump float;
  uniform float alpha;
  out vec4 frag_color;
  void main() {
    vec3 color = vec3(0.39, 0.86, 0.97);
    frag_color = vec4(color * alpha, alpha);
  }
);
// clang-format on
}  // namespace

LassoRenderer::~LassoRenderer() {
  if (!this->initialized_) {
    return;
  }
  glDeleteProgram(this->program_);
  glDeleteShader(this->vert_shader_);
  glDeleteShader(this->frag_shader_);
  glDeleteVertexArrays(1, &this->vao_);
  glDeleteBuffers(1, &this->vbo_);
}

void LassoRenderer::render(const glm::mat4& vp_mat,
    const std::vector<RayState>& lasso_points, float alpha) {
  if (!this->initialized_) {
    this->init();
  }

  std::vector<glm::vec3> vertices;
  vertices.reserve(lasso_points.size() + 1);
  const auto& origin = server::get().ray_cursor.origin();
  const auto  distance =
      server::get().setting.config.get<"surface_grouping", "line_distance">();
  for (const auto& point : lasso_points) {
    const auto rotation =
        glm::angleAxis(-point.azimuth, glm::vec3{0.F, 1.F, 0.F}) *
        glm::angleAxis(+point.polar, glm::vec3{1.F, 0.F, 0.F});
    const auto direction = glm::rotate(rotation, RayCursor::kBaseDirection);
    vertices.emplace_back(origin + direction * static_cast<float>(distance));
  }
  // close lasso
  vertices.push_back(vertices.front());

  float original_line_width = 1.F;
  glGetFloatv(GL_LINE_WIDTH, &original_line_width);
  const auto line_width =
      server::get().setting.config.get<"surface_grouping", "line_width">();
  glLineWidth(static_cast<float>(line_width));

  const auto size = static_cast<GLsizei>(vertices.size());
  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glBufferData(GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(size * sizeof(glm::vec3)), vertices.data(),
      GL_DYNAMIC_DRAW);

  glUseProgram(this->program_);
  glUniformMatrix4fv(
      this->vp_mat_location_, 1, GL_FALSE, glm::value_ptr(vp_mat));
  glUniform1f(this->alpha_location_, alpha);
  glDrawArrays(GL_LINE_STRIP, 0, size);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
  glLineWidth(original_line_width);
}

void LassoRenderer::init() {
  // If compilation fails, the compositor won't work,
  // so use `.value()` so that it throws an exception
  this->vert_shader_ = compile_glsl(GL_VERTEX_SHADER, vert_shader).value();
  this->frag_shader_ = compile_glsl(GL_FRAGMENT_SHADER, frag_shader).value();
  this->program_     = glCreateProgram();
  glAttachShader(this->program_, this->vert_shader_);
  glAttachShader(this->program_, this->frag_shader_);
  if (!link_program(this->program_)) {
    throw std::runtime_error("Failed to link LassoRenderer shader program");
  }
  this->vp_mat_location_ = glGetUniformLocation(this->program_, "vp_mat");
  this->alpha_location_  = glGetUniformLocation(this->program_, "alpha");

  glGenVertexArrays(1, &this->vao_);
  glGenBuffers(1, &this->vbo_);

  glBindVertexArray(this->vao_);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  this->initialized_ = true;
}
}  // namespace yabe::core::render
