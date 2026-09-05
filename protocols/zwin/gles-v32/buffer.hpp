#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <zwin-gles-v32-protocol.h>

#include <memory>
#include <mutex>

#include "common/common.hpp"
#include "common/weak-resource.hpp"

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
  };
  std::shared_ptr<State> pending_, committed_;

  GLuint     buffer_ = 0;
  std::mutex mutex_;
};

void create_buffer(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
