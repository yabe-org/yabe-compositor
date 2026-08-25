#pragma once

#include <GLES3/gl32.h>

#include <glm/glm.hpp>

#include "common/common.hpp"
#include "texture.hpp"

namespace yabe::core::render {
class RectRenderer {
 public:
  DISABLE_MOVE_AND_COPY(RectRenderer);
  // initialization must be done in the rendering thread, so we don't initialize
  // in the constructor. instead, we will initialize at the beginning of
  // rendering_scope function
  explicit RectRenderer() = default;
  ~RectRenderer();

  /// must be called in the rendering thread
  void render(const glm::mat4& mvp_mat, const Texture& texture);

 private:
  bool initialized_ = false;
  void init();

  GLuint vert_shader_              = 0;
  GLuint frag_shader_              = 0;
  GLuint program_                  = 0;
  GLint  mvp_mat_location_         = 0;
  GLint  texture_sampler_location_ = 0;

  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};
}  // namespace yabe::core::render
