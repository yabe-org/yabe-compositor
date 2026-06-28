#pragma once

#include <GLES3/gl3.h>

#include "common/common.hpp"

namespace yabe::core::render {
class Texture {
 public:
  DISABLE_MOVE_AND_COPY(Texture);
  explicit Texture();
  ~Texture();

  [[nodiscard]] GLuint get_texture_id() const {
    return this->texture_;
  }
  void upload(void* data, uint32_t wl_shm_format, int32_t width, int32_t height,
      int32_t stride);

 private:
  GLuint texture_ = 0;
};
}  // namespace yabe::core::render
