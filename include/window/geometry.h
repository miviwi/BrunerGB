#pragma once

#include <types.h>
#include <util/primitive.h>

namespace brdrive {

struct Geometry {
  u16 x, y;
  u16 w, h;

  static auto xy(u16 x, u16 y) -> Geometry
  {
    return { x, y, 0xFFFF, 0xFFFF };
  }
};

template <typename T>
struct Vec2 {
  T x, y;

  Vec2(T x_, T y_) : x(x_), y(y_) { }
  Vec2(T v) : x(v), y(v) { }

  static auto zero() -> Vec2
  {
    return { (T)0, (T)0 };
  }
};

using ivec2 = Vec2<int>;
using vec2 = Vec2<float>;

}
