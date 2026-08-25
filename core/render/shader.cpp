#include "shader.hpp"

#include <optional>
#include <vector>

#include "common/common.hpp"

namespace yabe::core::render {
std::optional<GLuint> compile_glsl(GLenum type, const char* src) {
  GLuint shader_id = glCreateShader(type);

  glShaderSource(shader_id, 1, &src, nullptr);
  glCompileShader(shader_id);

  GLint compiled = GL_FALSE;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
  if (compiled) {
    return shader_id;
  }
  int log_length = 0;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

  std::vector<char> log_message(log_length);
  glGetShaderInfoLog(shader_id, log_length, nullptr, log_message.data());
  LOG_ERR("[Shader error]\n{}", log_message.data());
  glDeleteShader(shader_id);
  return std::nullopt;
}

bool link_program(GLuint program) {
  glLinkProgram(program);

  GLint linked = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked) {
    return true;
  }

  int log_length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

  std::vector<char> log_message(log_length);
  glGetProgramInfoLog(program, log_length, nullptr, log_message.data());
  LOG_ERR("[Program link error]\n{}", log_message);
  return false;
}
}  // namespace yabe::core::render
