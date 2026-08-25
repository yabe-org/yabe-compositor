#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>

#include <functional>
#include <mutex>
#include <variant>

#include "common/common.hpp"
#include "common/weak-resource.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Texture {
 public:
  Texture() = default;
  DISABLE_MOVE_AND_COPY(Texture);
  ~Texture();

  void image_2d(GLenum target, GLint level, GLint internal_format,
      GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
      wl_resource* zwn_buffer_resource);
  void generate_mipmap(GLenum target);
  void commit();
  void bind(GLenum target);

 private:
  struct Image2D {
    GLenum               target;
    GLint                level;
    GLint                internal_format;
    GLsizei              width;
    GLsizei              height;
    GLint                border;
    GLenum               format;
    GLenum               type;
    common::WeakResource data;  // zwn_buffer
  };
  struct GenerateMipmap {
    GLenum target;
  };
  using Command = std::variant<Image2D, GenerateMipmap>;

  GLuint                             texture_ = 0;
  std::mutex                         mutex_;
  // functions should be executed while texture is bound
  std::vector<std::function<void()>> pending_;
  std::vector<std::function<void()>> committed_;
};

void create_texture(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
