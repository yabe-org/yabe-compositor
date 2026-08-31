#pragma once

#include <GLES3/gl32.h>

#include "common/common.hpp"
#include "common/unit.hpp"

namespace yabe::core::render {
class Texture {
 public:
  DISABLE_MOVE_AND_COPY(Texture);
  // initialization must be done in the rendering thread
  // so texture_ will be created at upload()
  explicit Texture() = default;
  ~Texture();

  [[nodiscard]] GLuint get_texture_id() const {
    return this->texture_;
  }

  /// must be called in the rendering thread
  void upload(void* data, uint32_t wl_shm_format,
      const common::unit::Pixel& width, const common::unit::Pixel& height,
      int32_t stride);
  void invalidate();

 private:
  GLuint texture_ = 0;
};
}  // namespace yabe::core::render
