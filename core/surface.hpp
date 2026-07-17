#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <optional>

#include "common/common.hpp"
#include "common/coordinate.hpp"
#include "common/geometry.hpp"
#include "common/unit.hpp"
#include "core/render/texture.hpp"

namespace yabe::core {
class SurfaceRole {
 public:
  virtual ~SurfaceRole()                        = default;
  virtual void commit()                         = 0;
  virtual void set_activated(bool is_activated) = 0;
};

enum class SurfaceType : uint8_t {
  TOPLEVEL,
  CURSOR,
};

struct IntersectInfo {
  float     distance;
  glm::vec3 pos;
};

constexpr float kSurfaceDistance = 0.5F;

class Surface : public std::enable_shared_from_this<Surface> {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(wl_resource* resource);
  ~Surface();

  void set_callback(wl_resource* resource);
  void attach(wl_resource* resource);
  void commit();

  void upload_texture_if_needed();
  void finish_rendering();

  void set_role(std::unique_ptr<SurfaceRole>&& role) {
    this->role_ = std::move(role);
  }
  void set_type(SurfaceType type) {
    this->type_ = type;
  }

  void set_iter(std::list<std::shared_ptr<Surface>>::iterator it) {
    this->it_ = it;
  }
  std::list<std::shared_ptr<Surface>>::iterator get_self_iter() {
    return this->it_;
  }

  void move_absolute(const glm::vec3& pos);
  void move_relative(float polar, float azimuth);

  [[nodiscard]] std::optional<IntersectInfo> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction);

  void focus();
  void unfocus();

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] wl_client* client() const {
    return wl_resource_get_client(this->resource_);
  }
  [[nodiscard]] glm::mat4              model_mat();
  [[nodiscard]] const render::Texture& texture() const {
    return this->texture_;
  }
  [[nodiscard]] const unit::Pixel& texture_width() const {
    return this->texture_width_;
  }
  [[nodiscard]] const unit::Pixel& texture_height() const {
    return this->texture_height_;
  }
  [[nodiscard]] SurfaceType type() const {
    return this->type_;
  }

 private:
  Geometry    geometry_;
  unit::Pixel texture_width_{0}, texture_height_{0};

  SurfaceType type_ = SurfaceType::TOPLEVEL;

  struct {
    std::optional<wl_resource*> buffer;
    std::optional<wl_resource*> callback;
  } pending_, current_;
  render::Texture texture_;
  bool                        is_texture_uploaded_ = false;
  std::optional<wl_resource*> uploaded_buffer_;
  std::mutex                  buffer_mutex_;

  std::unique_ptr<SurfaceRole> role_;

  std::list<std::shared_ptr<Surface>>::iterator it_;
  wl_resource*                                  resource_;
};
}  // namespace yabe::core
