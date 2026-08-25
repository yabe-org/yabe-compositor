#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>

#include "common/common.hpp"
#include "program.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Shader {
 public:
  Shader(std::string&& source, GLenum type);
  DISABLE_MOVE_AND_COPY(Shader);
  ~Shader();

 private:
  GLuint      prepare();
  friend void Program::use(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

  std::string source_;
  GLenum      type_;
  bool        is_uploaded_ = false;
  GLuint      shader_      = 0;
};

void create_shader(wl_client* client, int version, uint32_t id,
    std::string&& source, GLenum type);
}  // namespace yabe::proto::zwin::gles_v32
