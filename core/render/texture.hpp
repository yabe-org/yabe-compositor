#pragma once

#include <GLES3/gl32.h>

#include <cstdint>
#include <vector>

#include "common/box.hpp"
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
  /// Upload only `damaged_boxes` of `data`.
  /// The whole data is uploaded if the texture has not been created
  /// or its size or format differs from the previous upload,
  /// because the texture does not have valid contents outside the damage.
  /// must be called in the rendering thread
  void upload_damaged(void* data, uint32_t wl_shm_format,
      const common::unit::Pixel& width, const common::unit::Pixel& height,
      int32_t stride, const std::vector<common::Box2D>& damaged_boxes);
  void invalidate();

 private:
  GLuint              texture_ = 0;
  // properties of the last upload
  common::unit::Pixel width_{0}, height_{0};
  uint32_t            wl_shm_format_ = 0;

  [[nodiscard]] bool can_update_partially(uint32_t wl_shm_format,
      const common::unit::Pixel&                   width,
      const common::unit::Pixel&                   height) const;
};
}  // namespace yabe::core::render
