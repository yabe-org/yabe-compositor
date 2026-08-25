#pragma once

#include <GLES3/gl32.h>
#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "common/common.hpp"

namespace yabe::proto::zwin::gles_v32 {
class Shader;

class Program {
 public:
  Program() = default;
  DISABLE_MOVE_AND_COPY(Program);
  ~Program();

  void  attach_shader(const std::shared_ptr<Shader>& shader);
  void  request_link();
  GLint get_location(const std::string& name);

  void commit();
  void use(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

  [[nodiscard]] bool is_linked() const {
    return this->state_ == State::LINKED;
  }
  [[nodiscard]] GLuint program() const {
    return this->program_;
  }

 private:
  enum class State : uint8_t {
    INITIAL,
    LINK_REQUESTED,
    WAIT_FOR_LINK,
    LINKED,
  } state_;
  std::vector<std::weak_ptr<Shader>> shaders_;

  GLuint     program_ = 0;
  std::mutex mutex_;
};

void create_program(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::gles_v32
