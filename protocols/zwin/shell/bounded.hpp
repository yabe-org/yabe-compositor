#pragma once

#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <memory>
#include <optional>

#include "common/common.hpp"
#include "core/scene/bounded-node.hpp"
#include "protocols/zwin/region.hpp"
#include "protocols/zwin/shell/app.hpp"
#include "protocols/zwin/virtual-object.hpp"

namespace yabe::proto::zwin::shell {
class Bounded : public std::enable_shared_from_this<Bounded>, public ZwinApp {
 public:
  DISABLE_MOVE_AND_COPY(Bounded);
  Bounded(wl_resource*                      resource,
      const std::shared_ptr<VirtualObject>& virtual_object,
      const glm::vec3&                      half_size);
  ~Bounded() override;

  void set_region(std::optional<compositor::Region*> region);
  void start_move();

  void draw(const glm::mat4& proj_mat, const glm::mat4& view_mat,
      const glm::mat4& model_mat);

  void map() override;
  void unmap() override;

  [[nodiscard]] wl_client* client() const {
    return wl_resource_get_client(this->resource_);
  }
  [[nodiscard]] std::weak_ptr<VirtualObject> virtual_object() const {
    return this->virtual_object_;
  }
  [[nodiscard]] const glm::vec3& half_size() const {
    return this->half_size_;
  }
  [[nodiscard]] std::optional<compositor::Region> region() const {
    return this->region_;
  }

 private:
  wl_resource*                 resource_;
  std::weak_ptr<VirtualObject> virtual_object_;
  glm::vec3                    half_size_;

  std::optional<compositor::Region> region_;

  std::optional<std::shared_ptr<core::scene::BoundedNode>> scene_node_ =
      std::nullopt;
};

void create_bounded(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object, wl_array* half_size);
}  // namespace yabe::proto::zwin::shell
