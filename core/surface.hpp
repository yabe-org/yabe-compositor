#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <cstdint>
#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <optional>

#include "common/common.hpp"
#include "common/pos.hpp"
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

class Surface {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(wl_resource* resource);
  ~Surface();

  void set_callback(wl_resource* resource);
  void attach(wl_resource* resource);
  void commit();

  void upload_texture_if_needed();
  void send_frame_done();

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

  static Surface* from(wl_resource* wl_surface);

  void move_absolute(const glm::vec3& pos);
  void move_relative(double dx, double dy);

  void motion(const common::SurfacePos& pos) const;
  void enter(const common::SurfacePos& pos) const;
  void leave() const;
  void frame() const;
  void button(
      uint32_t time, uint32_t button, wl_pointer_button_state state) const;
  void axis(uint32_t time, wl_pointer_axis axis, double value) const;

  void focus();
  void unfocus();

  [[nodiscard]] wl_resource* resource() const {
    return this->resource_;
  }
  [[nodiscard]] wl_client* client() const {
    return wl_resource_get_client(this->resource_);
  }
  [[nodiscard]] glm::mat4              model_mat() const;
  [[nodiscard]] const render::Texture& texture() const {
    return this->texture_;
  }
  [[nodiscard]] int32_t width() const {
    return this->width_;
  }
  [[nodiscard]] int32_t height() const {
    return this->height_;
  }
  [[nodiscard]] SurfaceType type() const {
    return this->type_;
  }

 private:
  glm::vec3 pos_;
  int32_t   width_ = 0, height_ = 0;

  SurfaceType type_ = SurfaceType::TOPLEVEL;

  struct {
    std::optional<wl_resource*> buffer;
    std::optional<wl_resource*> callback;
  } pending_, current_;
  render::Texture texture_;
  bool            is_texture_uploaded_ = false;

  std::unique_ptr<SurfaceRole> role_;

  std::list<std::shared_ptr<Surface>>::iterator it_;
  wl_resource*                                  resource_;
};
}  // namespace yabe::core
