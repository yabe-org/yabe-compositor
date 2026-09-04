#include "scene.hpp"

#include <memory>
#include <mutex>
#include <numbers>
#include <ranges>

#include "common/geometry.hpp"
#include "core/scene/scene-intersection.hpp"
#include "core/scene/scene-node.hpp"
#include "core/scene/surface-node.hpp"
#include "core/server.hpp"

namespace yabe::core::scene {
void Scene::insert(std::shared_ptr<SceneNode>&& node) {
  if (!this->active_node_.lock()) {
    this->set_active_node(node);
  }
  const std::lock_guard<std::mutex> lock(this->mutex_);
  this->nodes_.emplace_back(std::move(node));
}

void Scene::erase(const std::shared_ptr<SceneNode>& node) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (auto& it : this->nodes_) {
    if (it == node) {
      this->nodes_.remove(it);
      return;
    }
    if (it->try_erace_child(node)) {
      return;
    }
  }
}

void Scene::raise_to_front(const std::shared_ptr<SceneNode>& node) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  auto it = std::find(this->nodes_.begin(), this->nodes_.end(), node);
  if (it != this->nodes_.end()) {
    this->nodes_.splice(this->nodes_.end(), this->nodes_, it);
    return;
  }
  for (auto& it : this->nodes_) {
    if (it->try_raise_child_to_front(node)) {
      return;
    }
  }
}

void Scene::set_active_node(const std::shared_ptr<SceneNode>& node) {
  // do not lock mutex here because this sends Wayland related events only
  const auto current_active_node = this->active_node_.lock();
  if (node == current_active_node) {
    return;
  }
  if (const auto surface_node =
          std::dynamic_pointer_cast<SurfaceNode>(current_active_node)) {
    if (const auto toplevel = surface_node->surface()->get_toplevel()) {
      toplevel->set_activated(false);
    }
  }
  this->active_node_ = node;
  this->focus_to_active_node();
}
void Scene::focus_to_active_node() {
  const auto node = this->active_node_.lock();
  if (!node) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  this->raise_to_front(node);

  auto* data_device_target = node->data_device_target();
  if (data_device_target) {
    server::get().seat.data_device.send_selection(data_device_target);
  }

  const auto surface_node = std::dynamic_pointer_cast<SurfaceNode>(node);
  if (!surface_node) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  const auto toplevel = surface_node->surface()->get_toplevel();
  if (!toplevel) {
    server::get().seat.keyboard.set_focused_surface({});
    return;
  }
  server::get().seat.keyboard.set_focused_surface(toplevel->surface().lock());
  toplevel->set_activated(true);
}

std::list<std::shared_ptr<SceneNode>>
Scene::extract_lasso_selected_surface_nodes(std::span<RayState> lasso_points) {
  const std::lock_guard<std::mutex>     lock(this->mutex_);
  std::list<std::shared_ptr<SceneNode>> result;
  for (auto node_it = this->nodes_.begin(); node_it != this->nodes_.end();) {
    {
      const auto surface_node =
          std::dynamic_pointer_cast<SurfaceNode>(*node_it);
      if (!surface_node) {
        ++node_it;
        continue;
      }
      if (!surface_node->surface()->get_toplevel()) {
        ++node_it;
        continue;
      }
    }
    const auto node         = *node_it;
    float      node_polar   = 0.F;
    float      node_azimuth = 0.F;
    {
      auto node_geometry = common::Geometry(node->geometry());
      node_geometry.pos() -= server::get().ray_cursor.origin();
      auto node_spherical = node_geometry.cartesian().to_spherical();
      node_polar          = node_spherical.polar;
      // azimuth is in [-pi, +pi] but selection collapse near that boundary
      // so align near the first value of lasso_points
      node_azimuth = lasso_points.front().azimuth +
                     static_cast<float>(std::remainder(
                         node_spherical.azimuth - lasso_points.front().azimuth,
                         2.F * std::numbers::pi));
    }
    bool contains = false;
    // begin with the last element and the first element in order to
    // check the line segment who is finally added to close the lasso area
    // TODO: replace this with std::views::stride(N) | std::views::adjacent<2>
    //       to decrease the resolution of the lasso points and
    //       improve performance
    for (size_t i = 0, j = lasso_points.size() - 1; i < lasso_points.size();
        j = i++) {
      const auto& a = lasso_points[i];
      const auto& b = lasso_points[j];
      /* Assume that:
        polar
          ^
          | +-------+ <- lasso line
          | |         *A
          | | node*     +
          | |            *B
          | +--------------+
       ---+-------------> azimuth
         In this case,
           - node crosses the line segment A-B in polar direction
           - the ray casted from node to +azimuth direction
             crosses lasso lines for 1 time (odd number of times)
             - such ray always crosses the lasso for odd number of times
               when the node is inside the lasso area
             - if there is a node at the left side of the lasso,
               the ray from such node will cross 2 times with the lasso

         At this figure, the position of `+` position (on A-B line) is the
         intersection point of the ray casted from node to +azimuth direction
         Its position can be calculated as:
           - polar   = A.polar   + (B.polar   - A.polar  ) * X (1)
           - azimuth = A.azimuth + (B.azimuth - A.azimuth) * X (2)
         From (1), as polar == node_polar (because it is intersection point),
           - X = (node_polar - A.polar) / (B.polar - A.polar) (3)
         Therefore, if we know that node crosses the line segment A-B
         in polar direction, we can calculate
         the azimuth of the intersection point by:
           A.azimuth + (B.azimuth - A.azimuth) * X [therefore (2)]
         = A.azimuth + (B.azimuth - A.azimuth) *
             {(node_polar - A.polar) / (B.polar - A.polar)} [therefore (3)]
       */
      // LOG_DBG(
      //     "\n"  //
      //     "  A   : polar={}, azimuth={}\n"
      //     "  B   : polar={}, azimuth={}\n"
      //     "  node: polar={}, azimuth={}",
      //     a.polar, a.azimuth, b.polar, b.azimuth, node_polar, node_azimuth);
      const auto  polar_crossed =
          (a.polar > node_polar) != (b.polar > node_polar);
      if (!polar_crossed) {
        continue;
      }
      const auto delta_polar = b.polar - a.polar;
      if (delta_polar == 0.F) {
        continue;
      }
      const auto delta_azimuth = b.azimuth - a.azimuth;
      const auto intersection_azimuth =
          a.azimuth + (delta_azimuth * (node_polar - a.polar) / delta_polar);
      // intersection pos is at the right side of the node
      if (intersection_azimuth > node_azimuth) {
        contains = !contains;
      }
    }
    if (!contains) {
      ++node_it;
      continue;
    }
    result.splice(result.end(), this->nodes_, node_it++);
  }
  return result;
}

void Scene::draw(const glm::mat4& proj_mat, const glm::mat4& view_mat) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (const auto& node : this->nodes_) {
    node->draw(proj_mat, view_mat, nullptr, glm::mat4(1.F));
  }
}

std::optional<SceneIntersection> Scene::get_nearest_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  const std::lock_guard<std::mutex> lock(this->mutex_);
  for (const auto& node : this->nodes_ | std::views::reverse) {
    const auto intersection =
        node->check_intersection(origin, direction, nullptr, glm::mat4(1.F));
    if (intersection.has_value()) {
      return intersection;
    }
  }
  return std::nullopt;
}
}  // namespace yabe::core::scene
