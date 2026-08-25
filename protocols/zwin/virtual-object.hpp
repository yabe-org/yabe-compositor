#pragma once

#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "common/common.hpp"
#include "protocols/zwin/shell/app.hpp"

namespace yabe::proto::zwin {
namespace gles_v32 {
class RenderingUnit;
}

class VirtualObject {
 public:
  DISABLE_MOVE_AND_COPY(VirtualObject);
  explicit VirtualObject(wl_resource*);
  ~VirtualObject() = default;

  void set_parent(const std::shared_ptr<shell::ZwinApp>& parent);
  void add_frame_callback(wl_resource* callback_resource);
  void add_rendering_unit(const std::shared_ptr<gles_v32::RenderingUnit>& unit);

  void commit();
  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }

 private:
  struct {
    std::vector<wl_resource*> frame_callbacks;
  } pending_, current_;

  wl_resource* resource_;
  std::mutex   mutex_;

  std::vector<std::weak_ptr<gles_v32::RenderingUnit>> rendering_units_;
  std::weak_ptr<shell::ZwinApp>                       parent_;
};

void create_virtual_object(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin
