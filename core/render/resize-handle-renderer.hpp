#pragma once

#include <GLES3/gl32.h>

#include <glm/ext.hpp>

#include "common/common.hpp"

namespace yabe::core::render {
class ResizeHandleRenderer {
 public:
  DISABLE_MOVE_AND_COPY(ResizeHandleRenderer);
  // initialization must be done in the rendering thread,
  // so we don't initialize in the constructor
  ResizeHandleRenderer() = default;
  ~ResizeHandleRenderer();

  void render(
      const glm::mat4& mvp_mat, const glm::vec2& intersected_pos, float alpha);
  void invalidate();

 private:
  bool initialized_ = false;
  void init();

  GLuint vert_shader_              = 0;
  GLuint frag_shader_              = 0;
  GLuint program_                  = 0;
  GLint  mvp_mat_location_         = 0;
  GLint  alpha_location_           = 0;
  GLint  intersected_pos_location_ = 0;

  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};
}  // namespace yabe::core::render
