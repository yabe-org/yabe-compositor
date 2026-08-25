#pragma once

#include <GLES3/gl32.h>

#include <optional>

#define GLSL(s) (const char*)"#version 300 es\n" #s

namespace yabe::core::render {
std::optional<GLuint> compile_glsl(GLenum type, const char* src);
bool                  link_program(GLuint program);

}  // namespace yabe::core::render
