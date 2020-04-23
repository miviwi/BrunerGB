#include <gx/gx.h>
#include <gx/pipeline.h>
#include <gx/vertex.h>
#include <gx/program.h>

#include <algorithm>
#include <limits>

#include <cstdio>
#include <cstring>

// OpenGL/gl3w
#include <GL/gl3w.h>
#include <sys/cdefs.h>

namespace brgb {

static GLPipeline p_current_pipeline;

[[using gnu: always_inline]]
static constexpr auto GLType_to_index_buf_type(GLType type) -> GLEnum
{
  switch(type) {
  case GLType::u8:  return GL_UNSIGNED_BYTE;
  case GLType::u16: return GL_UNSIGNED_SHORT;
  case GLType::u32: return GL_UNSIGNED_INT;

  default: ;   // Silence warnings
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto GLPrimitive_to_mode(int p) -> GLEnum
{
  switch((GLPrimitive)p) {
  case GLPrimitive::Points:        return GL_POINTS;
  case GLPrimitive::Lines:         return GL_LINES;
  case GLPrimitive::LineStrip:     return GL_LINE_STRIP;
  case GLPrimitive::LineLoop:      return GL_LINE_LOOP;
  case GLPrimitive::Triangles:     return GL_TRIANGLES;
  case GLPrimitive::TriangleStrip: return GL_TRIANGLE_STRIP;
  case GLPrimitive::TriangleFan:   return GL_TRIANGLE_FAN;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto blend_factor_to_GLEnum(u32 factor) -> GLEnum
{
  switch(factor) {
  case GLPipeline::Factor0:                 return GL_ZERO;
  case GLPipeline::Factor1:                 return GL_ONE;
  case GLPipeline::FactorSrcColor:          return GL_SRC_COLOR;
  case GLPipeline::Factor1MinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
  case GLPipeline::FactorDstColor:          return GL_DST_COLOR;
  case GLPipeline::Factor1MinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
  case GLPipeline::FactorSrcAlpha:          return GL_SRC_ALPHA;
  case GLPipeline::Factor1MinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
  case GLPipeline::FactorDstAlpha:          return GL_DST_ALPHA;
  case GLPipeline::Factor1MinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
  case GLPipeline::FactorConstColor:        return GL_CONSTANT_COLOR;
  case GLPipeline::Factor1MinusConstColor:  return GL_ONE_MINUS_CONSTANT_COLOR;
  case GLPipeline::FactorConstAlpha:        return GL_CONSTANT_ALPHA;
  case GLPipeline::Factor1MinusConstAlpha:  return GL_ONE_MINUS_CONSTANT_ALPHA;
  case GLPipeline::FactorSrcAlpha_Saturate: return GL_SRC_ALPHA_SATURATE;
  }

  return GL_INVALID_ENUM;
}

auto GLPipeline::current() -> GLPipeline
{
  return p_current_pipeline;
}

GLPipeline::GLPipeline()
{
  std::fill(state_structs_.begin(), state_structs_.end(), std::monostate());
}

auto GLPipeline::use() -> GLPipeline&
{
  for(const auto& s : diff(p_current_pipeline.state_structs_)) {
    if((StateIndex)s.index() == StateIndex::None) break;  // diff() compacts the resulting array,
                                                          //   so we can early-out at the first
                                                          //   empty StateStruct
    switch((StateIndex)s.index()) {
    case StateIndex::VertexInput:   useVertexInputState(std::get<VertexInput>(s)); break;
    case StateIndex::InputAssembly: useInputAssemblyState(std::get<InputAssembly>(s)); break;
    case StateIndex::Viewport:      useViewportState(std::get<Viewport>(s)); break;
    case StateIndex::Scissor:       useScissorState(std::get<Scissor>(s)); break;
    case StateIndex::Rasterizer:    useRasterizerState(std::get<Rasterizer>(s)); break;
    case StateIndex::DepthStencil:  useDepthStencilState(std::get<DepthStencil>(s)); break;
    case StateIndex::Blend:         useBlendState(std::get<Blend>(s)); break;

    default: assert(0 && "GLPipeline::use() unexpected StateStruct type!");
    }
  }

  return p_current_pipeline = *this;
}

auto GLPipeline::draw(
    u32 count,
    size_t offset, size_t instance_count
  ) -> GLPipeline&
{
  auto pvertex_input = getState<VertexInput>();
  assert(pvertex_input && "GLPipeline::draw() called without VertexInput state set!");

  auto pinput_assembly = getState<InputAssembly>();
  assert(pinput_assembly && "GLPipeline::draw() called without InputAssembly state set!");

  auto& vi = *pvertex_input;
  auto& ia = *pinput_assembly;

  auto mode = GLPrimitive_to_mode(ia.primitive);
  
  // Sanity check
  assert(offset < std::numeric_limits<GLint>::max());

  if(!instance_count) {
    glDrawArrays(mode, (GLint)offset, count);
  } else {
    glDrawArraysInstanced(mode, (GLint)offset, count, instance_count);
  }

  return *this;
}

auto GLPipeline::drawIndexed(
    u32 count,
    size_t offset, size_t instance_count
  ) -> GLPipeline&
{
  auto pvertex_input = getState<VertexInput>();
  assert(pvertex_input && "GLPipeline::draw() called without VertexInput state set!");

  auto pinput_assembly = getState<InputAssembly>();
  assert(pinput_assembly && "GLPipeline::draw() called without InputAssembly state set!");

  auto& vi = *pvertex_input;
  auto& ia = *pinput_assembly;

  auto mode = GLPrimitive_to_mode(ia.primitive);
  auto inds_type = GLType_to_index_buf_type(vi.indices_type);
  auto offset_ptr = (void *)(uintptr_t)offset;

  if(!instance_count) {
    glDrawElements(mode, count, inds_type, offset_ptr);
  } else {
    glDrawElementsInstanced(mode, count, inds_type, offset_ptr, instance_count);
  }

  return *this;
}

auto GLPipeline::drawIndexedBaseVertex(
    u32 count, int base_vertex,
    size_t offset, size_t instance_count
  ) -> GLPipeline&
{
  auto pvertex_input = getState<VertexInput>();
  assert(pvertex_input && "GLPipeline::draw() called without VertexInput state set!");

  auto pinput_assembly = getState<InputAssembly>();
  assert(pinput_assembly && "GLPipeline::draw() called without InputAssembly state set!");

  auto& vi = *pvertex_input;
  auto& ia = *pinput_assembly;

  auto mode = GLPrimitive_to_mode(ia.primitive);
  auto inds_type = GLType_to_index_buf_type(vi.indices_type);
  auto offset_ptr = (void *)(uintptr_t)offset;

  if(!instance_count) {
    glDrawElementsBaseVertex(mode, count, inds_type, offset_ptr, base_vertex);
  } else {
    glDrawElementsInstancedBaseVertex(mode, count, inds_type, offset_ptr, instance_count, base_vertex);
  }

  return *this;
}

auto GLPipeline::diff(const StateStructArray& other) -> StateStructArray
{
  auto order_state_structs = [](const StateStructArray& array) -> StateStructArray {
    StateStructArray ordered;

    std::fill(ordered.begin(), ordered.end(), std::monostate());
    for(const auto& st : array) {
      if(std::holds_alternative<std::monostate>(st)) continue;

      ordered[st.index() - 1] = st;  // StateIndex::None (the null StateStruct) does not
    }                                //   have a slot reserved in StateStructArray

    return ordered;
  };

  StateStructArray ordered = order_state_structs(state_structs_);
  StateStructArray other_ordered = order_state_structs(other);

  StateStructArray difference;
  std::fill(difference.begin(), difference.end(), std::monostate());

  for(size_t i = 0; i < difference.size(); i++) {
    const auto& a = ordered[i];
    const auto& b = other_ordered[i];

    const void *a_ptr = &a;
    const void *b_ptr = &b;

    auto result = memcmp(a_ptr, b_ptr, sizeof(StateStruct));

    // The structs are equal - so move on
    if(!result) continue;

    // Otherwise - add 'b' to the difference
    difference[i] = a;
  }

  // Compact the result (move all filled StateStructs to the beginning)
  std::remove_if(difference.begin(), difference.end(), [](const StateStruct& s) {
      return (StateIndex)s.index() == StateIndex::None;
  });
  
  return difference;
}

auto GLPipeline::useVertexInputState(const VertexInput& vi) -> void
{
  glBindVertexArray(vi.array);
}

auto GLPipeline::useInputAssemblyState(const InputAssembly& ia) -> void
{
  if(ia.primitive_restart) {
    glEnable(GL_PRIMITIVE_RESTART);

    glPrimitiveRestartIndex(ia.restart_index);
  } else {
    glDisable(GL_PRIMITIVE_RESTART);
  }
}

auto GLPipeline::useViewportState(const Viewport& v) -> void
{
  glViewport(v.x, v.y, v.w, v.h);
}

auto GLPipeline::useScissorState(const Scissor& s) -> void
{
  if(s.scissor) {
    glEnable(GL_SCISSOR_TEST);

    glScissor(s.x, s.y, s.w, s.h);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
}

auto GLPipeline::useRasterizerState(const Rasterizer& r) -> void
{
  if(r.cull_mode == CullNone) {
    glDisable(GL_CULL_FACE);
  } else {
    glEnable(GL_CULL_FACE);
  }

  switch(r.cull_mode) {
  case CullFront:        glCullFace(GL_FRONT); break;
  case CullBack:         glCullFace(GL_BACK); break;
  case CullFrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
  }

  switch(r.front_face) {
  case FrontFaceCCW: glFrontFace(GL_CCW); break;
  case FrontFaceCW:  glFrontFace(GL_CW); break;
  }

  switch(r.polygon_mode) {
  case PolygonModeFilled: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
  case PolygonModeLines:  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
  case PolygonModePoints: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
  }
}

auto GLPipeline::useDepthStencilState(const DepthStencil& ds) -> void
{
  if(ds.depth_test) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  switch(ds.depth_func) {
  case CompareFuncNever:        glDepthFunc(GL_NEVER); break;
  case CompareFuncAlways:       glDepthFunc(GL_ALWAYS); break;
  case CompareFuncEqual:        glDepthFunc(GL_EQUAL); break;
  case CompareFuncNotEqual:     glDepthFunc(GL_NOTEQUAL); break;
  case CompareFuncLess:         glDepthFunc(GL_LESS); break;
  case CompareFuncLessEqual:    glDepthFunc(GL_LEQUAL); break;
  case CompareFuncGreater:      glDepthFunc(GL_GREATER); break;
  case CompareFuncGreaterEqual: glDepthFunc(GL_GEQUAL); break;
  }
}

auto GLPipeline::useBlendState(const Blend& b) -> void
{
  if(b.blend) {
    glEnable(GL_BLEND);

    auto sfactor = blend_factor_to_GLEnum(b.src_factor),
         dfactor = blend_factor_to_GLEnum(b.dst_factor);

    glBlendFunc(sfactor, dfactor);
  } else {
    glDisable(GL_BLEND);
  }
}

}
