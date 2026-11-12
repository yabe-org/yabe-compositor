#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "common/common.hpp"
#include "common/thread-safety.hpp"
#include "common/unit.hpp"
#include "core/buffer.hpp"
#include "core/region.hpp"
#include "core/render/texture.hpp"
#include "core/scene/surface-node.hpp"
#include "core/toplevel.hpp"

namespace yabe::core {
enum class SurfaceSetParentResult : uint8_t {
  SUCCEEDED,
  FAILED_SELF_IS_SAME_AS_OR_ANCESTOR_OF_NEW_PARENT,
  FAILED_OTHER,
};

class SurfaceRole {
 public:
  DISABLE_MOVE_AND_COPY(SurfaceRole);
  virtual ~SurfaceRole()                                         = default;
  virtual void                                    commit()       = 0;
  [[nodiscard]] virtual std::shared_ptr<Toplevel> get_toplevel() = 0;

 protected:
  SurfaceRole() = default;
};

class Surface : public std::enable_shared_from_this<Surface> {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(wl_resource* resource);
  ~Surface();

  void set_frame_callback(wl_resource* resource);
  void set_input_region(std::optional<Region> region);
  void set_release_callback(wl_resource* resource);
  void attach(wl_resource* resource);
  /// x, y, width, height are buffer coordinates
  void damage(int32_t x, int32_t y, int32_t width, int32_t height);
  void commit();

  void upload_texture_if_needed();

  void set_role(std::weak_ptr<SurfaceRole>&& role) {
    this->role_ = std::move(role);
  }

  /// Create SurfaceNode and insert it into Scene
  void map(
      std::optional<std::shared_ptr<scene::SceneNode>> parent = std::nullopt,
      scene::SurfaceNode::Anchor anchor = scene::SurfaceNode::Anchor::CENTER);
  void                   unmap();
  SurfaceSetParentResult set_parent(
      std::optional<std::shared_ptr<Surface>> new_parent);

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
  [[nodiscard]] const common::unit::Pixel& texture_width() const {
    return this->texture_width_;
  }
  [[nodiscard]] const common::unit::Pixel& texture_height() const {
    return this->texture_height_;
  }
  [[nodiscard]] const std::optional<Region>& input_region() const {
#pragma clang diagnostic push
    // input_region is accessed only from Wayland thread
#pragma clang diagnostic ignored "-Wthread-safety"
    return this->current_.input_region;
#pragma clang diagnostic pop
  }
  [[nodiscard]] std::shared_ptr<Toplevel> get_toplevel() {
    if (const auto role = this->role_.lock()) {
      return role->get_toplevel();
    }
    return nullptr;
  }
  [[nodiscard]] bool is_ancestor_of(
      const std::shared_ptr<Surface>& other) const;

  [[nodiscard]] bool has_buffer() {
    const auto lock = this->mutex_.lock();
    return this->current_.buffer.has_value();
  }

 private:
  common::unit::Pixel texture_width_{0}, texture_height_{0};

  /// must be called in the rendering thread without locking mutex
  /// returns false if the buffer has been destroyed
  bool upload_texture(Buffer& buffer, Region& damage);

  common::Mutex mutex_;
  struct {
    // even if the buffer is not nullopt, Buffer::resource is nullable
    // in order to distinguish between "attach() is not called"
    // and "null buffer is attached (the content is removed)"
    std::optional<std::shared_ptr<Buffer>> buffer;
    // if pending_.input_region is nullopt and:
    //   - has_pending_input_region_ == true  : the input region is cleared
    //   - has_pending_input_region_ == false : the input region is not changed
    std::optional<Region>                  input_region;
    std::vector<wl_resource*>              frame_callbacks;
    std::optional<wl_resource*>            release_callback;
    // in buffer coordinates
    //   - pending_ : discarded on commit if no buffer is attached
    //   - current_ : the union of damages of all commits since the last
    //                upload, not only the latest one.
    //                If buffer A and then buffer B are committed before the
    //                next upload, A is never uploaded and the texture still
    //                holds the contents before A. So B must be uploaded in
    //                both A's and B's damage.
    Region                                 damage;
  } pending_, current_ GUARDED_BY(Surface::mutex_);
  bool has_pending_input_region_ = false;

  render::Texture         texture_;
  bool                    is_texture_uploaded_ = false;
  std::shared_ptr<Buffer> uploaded_buffer_;

  std::weak_ptr<SurfaceRole>                         role_;
  std::weak_ptr<Surface>                             parent_;
  std::optional<std::shared_ptr<scene::SurfaceNode>> surface_node_ =
      std::nullopt;

  wl_resource* resource_;
};
}  // namespace yabe::core
