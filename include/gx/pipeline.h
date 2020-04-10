#pragma once

#include <gx/gx.h>

#include <array>
#include <variant>

namespace brdrive {

class GLPipeline {
public:
  enum : u32 {
    RestartIndexNone = 0u,
  };

  enum : u32 {
    CullNone,
    CullFront = (1<<0), CullBack = (1<<1), CullFrontAndBack = CullFront|CullBack,

    FrontFaceCCW = 0, FrontFaceCW = 1,

    PolygonModeFilled = 0, PolygonModeLines = 1, PolygonModePoints = 2,

    CompareFuncNever = 0, CompareFuncAlways = 1,
    CompareFuncEqual = 2, CompareFuncNotEqual = 3,
    CompareFuncLess = 4, CompareFuncLessEqual = 5,
    CompareFuncGreater = 6, CompareFuncGreaterEqual = 7,
  };

  struct Program {
    GLId program;
  };

  struct VertexInput {
    GLId array;
    GLType indices_type;
  };

  struct InputAssembly {
    GLPrimitive primitive;

    u32 restart_index;
  };

  struct Viewport {
    u16 x, y, w, h;
  };

  struct Scissor {
    u16 x, y, w, h;
  };

  struct Rasterizer {
    u32 cull_mode : 2;
    u32 cull_face : 1;
    u32 polygon_mode : 2;
  };

  // TODO: stencil test parameters
  struct DepthStencil {
    u32 depth_test : 1;
    u32 depth_func : 3;
  };

  struct Blend {
    u32 blend : 1;
  };

  struct RawStateStruct {
    // 2 u32's of space used at most by each of the possible variants
    u8 raw[2 * sizeof(u32)];
  };

  using StateStruct = std::variant<
    std::monostate,
    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,
    RawStateStruct
  >;

  GLPipeline();
  GLPipeline(const GLPipeline&) = delete;

private:
  using StateStructArray = std::array<StateStruct, std::variant_size_v<StateStruct>>;

  auto diff(const StateStructArray& other) -> StateStructArray;

  StateStructArray state_structs_;
};

}
