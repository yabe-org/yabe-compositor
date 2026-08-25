#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>

#include <functional>
#include <mutex>
#include <vector>

#include "common/common.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Sampler {
 public:
  DISABLE_MOVE_AND_COPY(Sampler);
  explicit Sampler();
  ~Sampler() = default;

  void commit();
  void bind(GLuint unit);

  void enqueue_parameter_task(uint32_t pname, GLfloat param);
  void enqueue_parameter_task(uint32_t pname, GLint param);
  void enqueue_parameter_task(uint32_t pname, std::vector<GLfloat>&& param);
  void enqueue_parameter_task(uint32_t pname, std::vector<GLint>&& param);
  void enqueue_parameter_task_int(uint32_t pname, std::vector<GLint>&& param);
  void enqueue_parameter_task_int(uint32_t pname, std::vector<GLuint>&& param);

 private:
  struct {
    std::vector<std::function<void()>> tasks_on_rendering_thread;
  } pending_, committed_;

  GLuint     sampler_ = 0;
  std::mutex mutex_;
};

void create_sampler(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
