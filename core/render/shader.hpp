#pragma once

#include <GLES3/gl3.h>

#include <optional>

#define GLSL(s) (const char*)"#version 300 es\n" #s

namespace yabe::core::render {
std::optional<GLuint> compile_glsl(int type, const char* src);
bool                  link_program(GLuint program);

}  // namespace yabe::core::render
