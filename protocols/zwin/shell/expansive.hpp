#pragma once

#include <wayland-server-core.h>

#include <glm/glm.hpp>
#include <memory>
#include <optional>

#include "common/common.hpp"
#include "core/scene/expansive-node.hpp"
#include "protocols/zwin/region.hpp"
#include "protocols/zwin/shell/app.hpp"
#include "protocols/zwin/virtual-object.hpp"

namespace yabe::proto::zwin::shell {
class Expansive : public std::enable_shared_from_this<Expansive>,
                  public ZwinApp {
 public:
  DISABLE_MOVE_AND_COPY(Expansive);
  Expansive(wl_resource*                    resource,
      const std::shared_ptr<VirtualObject>& virtual_object);
  ~Expansive() override;

  void set_region(std::optional<compositor::Region*> region);

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
  [[nodiscard]] std::optional<compositor::Region> region() const {
    return this->region_;
  }

 private:
  wl_resource*                 resource_;
  std::weak_ptr<VirtualObject> virtual_object_;

  std::optional<compositor::Region> region_;

  std::optional<std::shared_ptr<core::scene::ExpansiveNode>> scene_node_ =
      std::nullopt;
};

void create_expansive(wl_client* client, int version, uint32_t id,
    const std::shared_ptr<VirtualObject>& virtual_object);
}  // namespace yabe::proto::zwin::shell
