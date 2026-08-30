#pragma once

#include <GLES3/gl32.h>

#include <glm/ext.hpp>

#include "common/common.hpp"
#include "core/ray-cursor.hpp"

namespace yabe::core::render {
class LassoRenderer {
 public:
  DISABLE_MOVE_AND_COPY(LassoRenderer);
  // initialization must be done in the rendering thread,
  // so we don't initialize in the constructor
  explicit LassoRenderer() = default;
  ~LassoRenderer();

  void render(const glm::mat4&     vp_mat,
      const std::vector<RayState>& lasso_points, float alpha);

 private:
  bool initialized_ = false;
  void init();

  GLuint vert_shader_     = 0;
  GLuint frag_shader_     = 0;
  GLuint program_         = 0;
  GLint  vp_mat_location_ = 0;
  GLint  alpha_location_  = 0;

  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};
}  // namespace yabe::core::render
