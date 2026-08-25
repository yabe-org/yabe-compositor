#pragma once

#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <memory>
#include <mutex>

#include "common/common.hpp"
#include "protocols/zwin/virtual-object.hpp"

namespace yabe::proto::zwin::gles_v32 {
class BaseTechnique;

class RenderingUnit {
 public:
  explicit RenderingUnit() = default;
  DISABLE_MOVE_AND_COPY(RenderingUnit);
  ~RenderingUnit() = default;

  /// @return false if the technique is already set
  bool set_technique(const std::shared_ptr<BaseTechnique>& technique);
  void commit();
  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

 private:
  std::mutex                   mutex_;
  std::weak_ptr<BaseTechnique> technique_;
};

void create_rendering_unit(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object);
}  // namespace yabe::proto::zwin::gles_v32
