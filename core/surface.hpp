#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <optional>

#include "common/common.hpp"
#include "common/unit.hpp"
#include "core/render/texture.hpp"
#include "core/scene/surface-node.hpp"

namespace yabe::core {
class SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(SurfaceRole);
  virtual ~SurfaceRole()                        = default;
  virtual void commit()                         = 0;
  virtual void set_activated(bool is_activated) = 0;

 protected:
  SurfaceRole() = default;
};

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

  void set_role(std::weak_ptr<SurfaceRole>&& role) {
    this->role_ = std::move(role);
  }

  /// Create SurfaceNode and insert it into Scene
  void map(
      std::optional<std::shared_ptr<scene::SceneNode>> parent = std::nullopt);
  void unmap();

  void focus();
  void unfocus();

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] wl_client* client() const {
    return wl_resource_get_client(this->resource_);
  }
  [[nodiscard]] std::optional<std::shared_ptr<scene::SurfaceNode>>
  surface_node() const {
    return this->surface_node_;
  }
  [[nodiscard]] const render::Texture& texture() const {
    return this->texture_;
  }
  [[nodiscard]] const unit::Pixel& texture_width() const {
    return this->texture_width_;
  }
  [[nodiscard]] const unit::Pixel& texture_height() const {
    return this->texture_height_;
  }

 private:
  unit::Pixel texture_width_{0}, texture_height_{0};

  struct {
    // even if the buffer is not nullopt, its pointer is nullable
    // in order to distinguish between "attach() is not called"
    // and "null buffer is attached (the content is removed)"
    std::optional<wl_resource*> buffer;
    std::optional<wl_resource*> callback;
  } pending_, current_;
  render::Texture             texture_;
  bool                        is_texture_uploaded_ = false;
  std::optional<wl_resource*> uploaded_buffer_;
  std::mutex                  buffer_mutex_;

  std::weak_ptr<SurfaceRole>                         role_;
  std::optional<std::shared_ptr<scene::SurfaceNode>> surface_node_ =
      std::nullopt;

  wl_resource* resource_;
};
}  // namespace yabe::core
