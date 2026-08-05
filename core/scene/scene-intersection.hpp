#pragma once

#include <memory>

#include "common/intersection.hpp"

namespace yabe::core::scene {
class SceneNode;
struct SceneIntersection {
  std::shared_ptr<SceneNode>          node;
  common::intersection::IntersectInfo intersect_info;
};
}  // namespace yabe::core::scene
