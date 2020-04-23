#pragma once

#include <gx/gx.h>

#include <type_traits>
#include <utility>
#include <array>
#include <variant>
#include <optional>

#include <cassert>

namespace brgb {

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

  struct VertexInput {
    GLId array;
    GLType indices_type = GLType::Invalid;

    auto with_array(GLId array_) -> VertexInput&
    {
      array = array_;

      return *this;
    }

    auto with_indexed_array(GLId array_, GLType inds_type) -> VertexInput&
    {
      array = array_;
      indices_type = inds_type;

      return *this;
    }
  };

  struct InputAssembly {
    u16 /* GLPrimitive  */ primitive;

    u16 primitive_restart = false;
    u32 restart_index;

    auto with_primitive(GLPrimitive prim) -> InputAssembly&
    {
      primitive = (u16)prim;

      return *this;
    }

    auto with_restart_index(u32 index) -> InputAssembly&
    {
      primitive_restart = true;
      restart_index = index;

      return *this;
    }
  };

  struct Viewport {
    u16 x, y, w, h;

    Viewport(u16 w_, u16 h_) :
      x(0), y(0), w(w_), h(h_)
    { }
    Viewport(u16 x_, u16 y_, u16 w_, u16 h_) :
      x(x_), y(y_), w(w_), h(h_)
    { }
  };

  struct Scissor {
    u32 scissor = false;
    u16 x, y, w, h;

    auto no_test() -> Scissor&
    {
      scissor = false;

      return *this;
    }

    auto with_test(u16 x_, u16 y_, u16 w_, u16 h_) -> Scissor&
    {
      scissor = true;
      x = x_;
      y = y_;
      w = w_;
      h = h_;

      return *this;
    }
  };

  struct Rasterizer {
    u32 cull_mode : 2;
    u32 front_face : 1;
    u32 polygon_mode : 2;

    auto no_cull_face(u32 poly_mode = PolygonModeFilled) -> Rasterizer&
    {
      cull_mode = CullNone;
      polygon_mode = poly_mode;

      return *this;
    }

    auto with_cull_face(
        u32 cull, u32 front_face_ = FrontFaceCCW,
        u32 poly_mode = PolygonModeFilled
      ) -> Rasterizer&
    {
      cull_mode = cull;
      front_face = front_face_;
      polygon_mode = poly_mode;

      return *this;
    }
  };

  // TODO: stencil test parameters
  struct DepthStencil {
    u32 depth_test : 1;
    u32 depth_func : 3;

    auto no_depth_test() -> DepthStencil&
    {
      depth_test = false;

      return *this;
    }

    auto with_depth_test(u32 func = CompareFuncLess) -> DepthStencil&
    {
      depth_test = false;
      depth_func = func;

      return *this;
    }
  };

  struct Blend {
    u32 blend : 1;

    auto no_blend() -> Blend&
    {
      blend = false;

      return *this;
    }
  };

  struct RawStateStruct {
    // 4 u32's of space at most allocated to one variant
    u8 raw[4 * sizeof(u32)];
  };

  using StateStruct = std::variant<
    std::monostate,
    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,

    RawStateStruct
  >;

  static auto current() -> GLPipeline;

  GLPipeline();
  GLPipeline(const GLPipeline&) = default;

  // Add a StateStruct to the configuration of this GLPipeline
  //  - When the first (and only) argument is a callable which matches
  //    the signature void(State&) it will be called with the created
  //    State object
  //  - Otherwise the method's arguments will be passed to the State
  //    object's constructor
  //  - A given StateStruct object can be add()'ed to a GLPipeline only
  //    ONCE!
  //
  //   Usage:
  //       auto pipeline = GLPipeline()
  //           .add<GLPipeline::Viewport>(1280, 720)
  //           .add<GLPipeline::InputAssembly>([](auto& ia) {
  //             ia.with_primitive(GLPrimitive::TraingleStrip);
  //           });
  //
  template <typename State, typename FnOrArg0, typename... Args>
  auto add(FnOrArg0&& fn_or_arg0, Args&&... args) -> GLPipeline&
  {
    assert(!getState<State>() &&
        "GLPipeline::add<State>() can be called for a given State only ONCE!");

    if constexpr(std::is_invocable_v<FnOrArg0, State&>) { // The first argument matches void(State&)
      static_assert(sizeof...(Args) == 0,
          "GLPipeline::add<State>() accepts EITHER a callable or constructor arguments (both given)");

      auto& state = state_structs_[num_state_structs_].emplace<State>();

      fn_or_arg0(state);
    } else {      // Pass the arguments to the constructor
      state_structs_[num_state_structs_].emplace<State>(
          std::forward<FnOrArg0>(fn_or_arg0), std::forward<Args>(args)...
      );
    }

    num_state_structs_++;

    return *this;
  }

  auto use() -> GLPipeline&;

  // Non-indexed draw methods
  auto draw(
      u32 count,
      size_t offset = 0, size_t instance_count = 0
    ) -> GLPipeline&;

  // Indexed draw methods
  auto drawIndexed(
      u32 count,
      size_t offset = 0, size_t instance_count = 0
    ) -> GLPipeline&;
  auto drawIndexedBaseVertex(
      u32 count, int base_vertex,
      size_t offset = 0, size_t instance_count = 0
    ) -> GLPipeline&;

private:
  using StateStructArray = std::array<StateStruct, std::variant_size_v<StateStruct>-2>;
  //                                               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //              Don't reserve slots for the null-StateStruct object and StateStructRaw

  enum class StateIndex {
    None,

    VertexInput, InputAssembly, Viewport, Scissor,
    Rasterizer, DepthStencil, Blend,

    RawStateStruct,
  };

  auto diff(const StateStructArray& other) -> StateStructArray;

  auto useVertexInputState(const VertexInput& vi) -> void;
  auto useInputAssemblyState(const InputAssembly& ia) -> void;
  auto useViewportState(const Viewport& v) -> void;
  auto useScissorState(const Scissor& s) -> void;
  auto useRasterizerState(const Rasterizer& r) -> void;
  auto useDepthStencilState(const DepthStencil& ds) -> void;
  auto useBlendState(const Blend& b) -> void;

  template <typename State>
  auto getState() -> State *
  {
    State *state = nullptr;
    for(size_t i = 0; i < num_state_structs_; i++) {   // Iterate only the filled entries
      auto& s = state_structs_[i];

      if(auto pstate = std::get_if<State>(&s)) {
        state = pstate;
        break;
      }
    }

    return state;
  }

  size_t num_state_structs_ = 0;

  StateStructArray state_structs_;
};

}
