#pragma once

#include <wayland-server-core.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace yabe::proto::zwin::compositor {
struct CuboidRegion {
  glm::vec3 half_size;
  glm::vec3 center;
  glm::quat rotation;
};

class Region {
 public:
  explicit Region()                = default;
  Region(const Region&)            = default;
  Region(Region&&)                 = delete;
  Region& operator=(const Region&) = default;
  Region& operator=(Region&&)      = delete;
  ~Region()                        = default;

  void add_cuboid(glm::vec3 half_size, glm::vec3 center, glm::quat rotation);
  [[nodiscard]] const std::vector<CuboidRegion>& cuboids() const {
    return this->cuboids_;
  }

 private:
  std::vector<CuboidRegion> cuboids_;
};

void create_region(wl_client* client, int version, uint32_t id);
}  // namespace yabe::proto::zwin::compositor
