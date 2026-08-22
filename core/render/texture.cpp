#include "texture.hpp"

#include <GLES3/gl3.h>
#include <wayland-server-protocol.h>

#include <optional>

namespace yabe::core::render {
namespace {
struct GlFormat {
  GLint  format;
  GLenum type;
  int    bytes_per_pixel;
  GLint  swizzle_r;
  GLint  swizzle_g;
  GLint  swizzle_b;
  GLint  swizzle_a;
};

// This does not cover all wl_shm_format values (fixme?)
std::optional<GlFormat> get_gl_format(uint32_t wl_shm_format) {
  switch (wl_shm_format) {
    case WL_SHM_FORMAT_ARGB8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_BLUE,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_RED,
          .swizzle_a       = GL_ALPHA,
      };
    case WL_SHM_FORMAT_XRGB8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_BLUE,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_RED,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_ABGR8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_RED,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_BLUE,
          .swizzle_a       = GL_ALPHA,
      };
    case WL_SHM_FORMAT_XBGR8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_RED,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_BLUE,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_RGBA8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_ALPHA,
          .swizzle_g       = GL_BLUE,
          .swizzle_b       = GL_GREEN,
          .swizzle_a       = GL_RED,
      };
    case WL_SHM_FORMAT_RGBX8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_ALPHA,
          .swizzle_g       = GL_BLUE,
          .swizzle_b       = GL_GREEN,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_BGRA8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_GREEN,
          .swizzle_g       = GL_BLUE,
          .swizzle_b       = GL_ALPHA,
          .swizzle_a       = GL_RED,
      };
    case WL_SHM_FORMAT_BGRX8888:
      return GlFormat{
          .format          = GL_RGBA,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 4,
          .swizzle_r       = GL_GREEN,
          .swizzle_g       = GL_BLUE,
          .swizzle_b       = GL_ALPHA,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_RGB888:
      return GlFormat{
          .format          = GL_RGB,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 3,
          .swizzle_r       = GL_RED,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_BLUE,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_BGR888:
      return GlFormat{
          .format          = GL_RGB,
          .type            = GL_UNSIGNED_BYTE,
          .bytes_per_pixel = 3,
          .swizzle_r       = GL_BLUE,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_RED,
          .swizzle_a       = GL_ONE,
      };
    case WL_SHM_FORMAT_RGB565:
      return GlFormat{
          .format          = GL_RGB,
          .type            = GL_UNSIGNED_SHORT_5_6_5,
          .bytes_per_pixel = 2,
          .swizzle_r       = GL_RED,
          .swizzle_g       = GL_GREEN,
          .swizzle_b       = GL_BLUE,
          .swizzle_a       = GL_ONE,
      };
    default:
      return std::nullopt;
  }
}
}  // namespace

Texture::~Texture() {
  if (this->texture_ != 0) {
    glDeleteTextures(1, &this->texture_);
  }
}

void Texture::upload(void* data, uint32_t wl_shm_format,
    const common::unit::Pixel& width, const common::unit::Pixel& height,
    int32_t stride) {
  auto gl_format = get_gl_format(wl_shm_format);
  if (!gl_format.has_value()) {
    LOG_ERR(
        "unsupported wl_shm_format: {} ({:x})", wl_shm_format, wl_shm_format);
    return;
  }

  if (this->texture_ == 0) {
    glGenTextures(1, &this->texture_);
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, this->texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, gl_format->swizzle_r);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, gl_format->swizzle_g);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, gl_format->swizzle_b);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, gl_format->swizzle_a);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / gl_format->bytes_per_pixel);
  glTexImage2D(GL_TEXTURE_2D, 0, gl_format->format, *width, *height, 0,
      gl_format->format, gl_format->type, data);
}
}  // namespace yabe::core::render
