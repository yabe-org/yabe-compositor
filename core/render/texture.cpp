#include "texture.hpp"

#include <GLES3/gl3.h>

namespace yabe::core::render {
Texture::~Texture() {
  if (this->texture_ != 0) {
    glDeleteTextures(1, &this->texture_);
  }
}

void Texture::upload(void* data, uint32_t wl_shm_format,
    const unit::Pixel& width, const unit::Pixel& height, int32_t stride) {
  if (this->texture_ == 0) {
    glGenTextures(1, &this->texture_);
  }
  glBindTexture(GL_TEXTURE_2D, this->texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  LOG_DBG("format: {} ({:x})", wl_shm_format, wl_shm_format);
  // FIXME: properly handle wl_shm_format by converting to OpenGL format
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 4 /*RGBA*/);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}
}  // namespace yabe::core::render
