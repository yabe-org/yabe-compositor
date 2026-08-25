#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include <mutex>

#include "common/common.hpp"
#include "common/weak-resource.hpp"
#include "zwin-gles-v32-protocol.h"

namespace yabe::proto::zwin::gles_v32 {
class Buffer {
 public:
  DISABLE_MOVE_AND_COPY(Buffer);
  Buffer() = default;
  ~Buffer();

  void set_data(GLenum target, wl_resource* data, GLenum usage);
  void commit();
  void bind(GLenum target);

 private:
  GLuint prepare();

  struct State {
    GLenum               target = -1;
    GLenum               usage  = -1;
    common::WeakResource data;  // zwn_buffer
    void                 reset() {
      this->target = -1;
      this->usage  = -1;
      this->data.set(nullptr);
    }
  } pending_, committed_;

  GLuint     buffer_ = 0;
  std::mutex mutex_;
};

void create_buffer(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
