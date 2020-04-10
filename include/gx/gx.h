#pragma once

#include <types.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

using GLEnum   = u32;
using GLId = unsigned;

using GLSize = int;
using GLSizePtr = intptr_t;

enum : GLId {
  GLNullId = 0u,
};

enum GLFormat : int {
  r, rg, rgb, rgba,
  r8, rg8, rgb8, rgba8,
  r16f, rg16f,
  r32f, rg32f,
  r8i, r8ui, r16i, r16ui,
  rg8i, rg8ui, rg16i, rg16ui,
  rgb8i, rgb8ui, rgb16i, rgb16ui,
  rgba8i, rgba8ui, rgba16i, rgba16ui,
  srgb8, srgb8_a8,
  depth,
  depth16, depth24, depth32f,
  depth_stencil,
  depth24_stencil8,
};

enum class GLType : int {
  Invalid,

  i8, i16, i32,
  u8, u16, u32,
  u16_565, u16_5551,
  u16_565r, u16_1555r,
  f16, f32, fixed16_16,

  u32_24_8,
  f32_u32_24_8r,
};

enum class GLPrimitive : int {
  Invalid,

  Points,
  Lines, LineStrip, LineLoop,
  Triangles, TriangleStrip, TriangleFan,
};

 // TODO: query OpenGL for this (?)
static constexpr unsigned GLNumTexImageUnits    = 16;
static constexpr unsigned GLNumBufferBindPoints = 16;

// Can only be called AFTER acquiring an OpenGL context!
void gx_init();
void gx_finalize();

auto gx_was_init() -> bool;

struct GL3WInitError : public std::runtime_error {
  GL3WInitError() :
    std::runtime_error("failed to initialize gl3w!")
  { }
};

}
