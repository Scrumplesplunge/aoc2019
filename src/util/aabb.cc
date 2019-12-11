export module util.aabb;

import "check.h";
import util.vec2;

export template <typename T>
struct aabb {
  aabb() = default;

  template <typename Container>
  aabb(const Container& values) {
    check(!values.empty());
    for (auto v : values) {
      min.x = std::min(min.x, v.x);
      min.y = std::min(min.y, v.y);
      max.x = std::max(max.x, v.x);
      max.y = std::max(max.y, v.y);
    }
  }

  // Both inclusive.
  vec2<T> min = {std::numeric_limits<T>::max(), std::numeric_limits<T>::max()},
          max = {std::numeric_limits<T>::min(), std::numeric_limits<T>::min()};
};
