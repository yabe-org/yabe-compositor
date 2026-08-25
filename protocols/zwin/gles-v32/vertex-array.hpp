#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/common.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Buffer;

class VertexArray {
 public:
  VertexArray() = default;
  DISABLE_MOVE_AND_COPY(VertexArray);
  ~VertexArray();

  void set_enabled(GLuint index, bool enabled);
  void set_attribute(GLuint index, GLint size, GLenum type,
      GLboolean normalized, GLsizei stride, off_t offset,
      const std::shared_ptr<Buffer>& buffer);
  void commit();
  void bind();

 private:
  void prepare();
  struct Attribute {
    bool                  enabled;
    GLint                 size;
    GLenum                type;
    GLboolean             normalized;
    GLsizei               stride;
    off_t                 offset;
    std::weak_ptr<Buffer> buffer;
  };
  using State = std::unordered_map<GLuint, Attribute>;

  GLuint     vertex_array_ = 0;
  std::mutex mutex_;
  State      pending_;
  State      committed_;
  State      current_;
};

void create_vertex_array(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
