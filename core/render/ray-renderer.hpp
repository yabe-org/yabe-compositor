#pragma once

#include <GLES3/gl32.h>

#include <glm/ext.hpp>

#include "common/common.hpp"

namespace yabe::core::render {
class RayRenderer {
 public:
  DISABLE_MOVE_AND_COPY(RayRenderer);
  // initialization must be done in the rendering thread,
  // so we don't initialize in the constructor
  explicit RayRenderer() = default;
  ~RayRenderer();

  void render(const glm::mat4& vp_mat, const glm::vec3& start_pos,
      const glm::vec3& direction, float length);

 private:
  bool initialized_ = false;
  void init();

  GLuint vert_shader_        = 0;
  GLuint frag_shader_        = 0;
  GLuint program_            = 0;
  GLint  vp_mat_location_    = 0;
  GLint  start_pos_location_ = 0;
  GLint  end_pos_location_   = 0;

  GLuint vao_ = 0;
  GLuint vbo_ = 0;
};
}  // namespace yabe::core::render
