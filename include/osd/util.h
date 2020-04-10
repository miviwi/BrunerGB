#pragma once

#include <osd/osd.h>

#include <array>

namespace brdrive {

using mat4 = std::array<float, 4*4>;

auto osd_ortho(float t, float l, float b, float r, float n, float f) -> mat4;

}
