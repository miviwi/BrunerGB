#include "gx/gx.h"
#include <osd/drawcall.h>
#include <osd/surface.h>

#include <gx/context.h>
#include <gx/vertex.h>
#include <gx/pipeline.h>
#include <gx/program.h>
#include <gx/buffer.h>
#include <gx/texture.h>
#include <gx/fence.h>

#include <utility>
#include <limits>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

namespace brgb {

[[using gnu: always_inline]]
constexpr static auto tex_and_sampler(
    GLTexture *tex, GLSampler *sampler
  ) -> OSDDrawCall::TextureAndSampler
{
  return OSDDrawCall::TextureAndSampler(tex, sampler);
}

[[using gnu: always_inline]]
constexpr static auto uniform_vec4(float x, float y, float z, float w) -> OSDDrawCall::UniformVec4
{
  return OSDDrawCall::UniformVec4({ x, y, z, w });
}

[[using gnu: always_inline]]
constexpr static auto uniform_ivec4(int x, int y, int z, int w) -> OSDDrawCall::UniformIVec4
{
  return OSDDrawCall::UniformIVec4({ x, y, z, w });
}

OSDDrawCall::OSDDrawCall() :
  command(DrawInvalid), type(DrawTypeInvalid),
  verts(nullptr), inds_type(GLType::Invalid),
  offset(-1), count(-1), instance_count(-1),
  textures_end(0),
  program(nullptr)
{
  for(auto& tex_and_sampler : textures) {
    tex_and_sampler = TextureAndSampler(nullptr, nullptr);
  }
}

auto osd_drawcall_strings(
    GLVertexArray *verts_, GLType inds_type_, GLIndexBuffer *inds_, GLSizePtr base_offset,
    GLSize max_string_len_, GLSize num_strings_,
    GLTexture2D *font_tex_, GLSampler *font_sampler_, GLTextureBuffer *strings_, GLTextureBuffer *attrs_
  ) -> OSDDrawCall
{
  auto drawcall = OSDDrawCall();

  drawcall.command = OSDDrawCall::DrawIndexedInstanced;
  drawcall.type    = OSDDrawCall::DrawString;

  drawcall.verts = verts_;
  drawcall.inds_type = inds_type_; drawcall.inds = inds_; drawcall.offset = 0;

  // Each glyph consists of a triangle fan with 4 vertices,
  //   which form a quad and a 5th vertex which is the
  //   PRIMITIVE_RESTART_INDEX
  drawcall.count = max_string_len_*5;

  drawcall.base_instance = base_offset;
  drawcall.instance_count = num_strings_;

  // Ordered in the way expected by the DrawType::DrawString shader
  drawcall.textures = OSDDrawCall::TextureBindings {
      tex_and_sampler(font_tex_, font_sampler_),
      tex_and_sampler(strings_, nullptr),
      tex_and_sampler(attrs_, nullptr),
    },

  drawcall.textures_end = 3; // 3 Textures sequentially - thus index '3' is one past the last one

  return drawcall;
}

auto osd_drawcall_quad(
    GLVertexArray *verts_,
    ivec2 pos, ivec2 width_height,
    GLTexture2D *textures[], GLSampler *samplers[], size_t num_textures,
    GLProgram *program_
  ) -> OSDDrawCall
{
  auto drawcall = OSDDrawCall();

  drawcall.command = OSDDrawCall::DrawArray;
  drawcall.type    = OSDDrawCall::DrawShadedQuad;

  drawcall.verts = verts_;
  drawcall.offset = 0;

  drawcall.inds = nullptr;   // Non-indexed draw

  // The quad is drawn as a triangle-fan, so there is no need to repeat vertices
  drawcall.count = 4;

  for(size_t t = 0; t < num_textures; t++) {
    drawcall.textures[t] = tex_and_sampler(textures[t], samplers[t]);
  }
  drawcall.textures_end = num_textures; // The textures are bound sequentially

  drawcall.program = program_;

  drawcall.uniforms.emplace({
      OSDDrawCall::ProgramUniform {
        .name = "uv4Quad_Pos_Dimensions",
        .val = uniform_ivec4(pos.x, pos.y, width_height.x, width_height.y),
      },
  });

  return drawcall;
}

auto osd_submit_drawcall(
    GLContext& gl_context, const OSDDrawCall& drawcall
  ) -> GLFence
{
  return std::move(drawcall.submit(OSDDrawCall::SubmitFriendKey(), gl_context));
}

// The inner array include a sentinel 'nullptr' at the end of the names
static const char *s_uniform_names[OSDDrawCall::NumDrawTypes][16 /* aribitrary */] = {
  // OSDSurface::DrawTypeInvalid
  { nullptr },

  // OSDSurface::DrawString
  { "usFont", "usStrings", "usStringAttributes", nullptr },

  // OSDSurface::DrawRectangle (TODO!)
  { nullptr },

  // OSDSurface::DrawShadedQuad (TODO!)
  { nullptr },
};

auto OSDDrawCall::submit(SubmitFriendKey, GLContext& gl_context) const -> GLFence
{
  assert((command != DrawInvalid && type != DrawTypeInvalid) &&
      "attempted to submit an invalid OSDDrawCall!");

  // If a program wasn't specified for this drawcall - use the
  //   default for it's 'type'
  auto& program = this->program ? *this->program : OSDSurface::renderProgram(type);
  program
    .use();

  switch(type) {
  case OSDDrawCall::DrawString:
    program.uniform("uiStringAttributesBaseOffset", (int)base_instance);
    break;
  }

  if(uniforms.has_value()) {
    for(const auto& u : uniforms.value()) {
      if((UniformValueType)u.val.index() == UniformValueType::IVec4) {
        auto& vec = std::get<UniformIVec4>(u.val);
        auto [x, y, z, w] = vec;

        program.uniformIVec(u.name.data(), x, y, z, w);
      }
    }
  }

  // Bind all the required textures and upload the TexImageUnit
  //   uniforms to the OSDSurface::renderProgram(type)...
  auto tex_uniform_names = s_uniform_names[type];
  for(unsigned i = 0; i < textures_end; i++) {
    auto [tex, sampler] = textures.at(i);
    if(!tex) continue;    // Empty slot...

    auto& tex_image_unit = gl_context.texImageUnit(i);
    if(!sampler) {
      tex_image_unit.bind(*tex);
    } else /* tex && sampler */ {
      tex_image_unit.bind(*tex, *sampler);
    }

    // Grab the name of this texture's sampler in the program...
    auto uniform_name = tex_uniform_names[i];
    assert(uniform_name);

    // ...and set it (i.e. the sampler/tex image unit)
    program
      .uniform(uniform_name, tex_image_unit);
  }

  // ...then the vertex array and index buffer...
  assert(verts && "attempted to submit an OSDDrawCall with a null vertex array!");
  assert((count >= 0 && offset >= 0) &&
      "attempted to submit an OSDDrawCall with a negative vertex count or offset!");
  assert((command != DrawIndexed && command != DrawIndexedInstanced) ||
        (inds_type != GLType::Invalid && inds && instance_count >= 0) &&
      "attempted to submit an indexed draw call with an invalid index buffer supplied!");

  auto pipeline = GLPipeline()
    .add<GLPipeline::VertexInput>([this](GLPipeline::VertexInput& vi) {
        vi.with_indexed_array(0 /* TODO: pass GLVertexArray here :) */, inds_type);
    })
    .add<GLPipeline::InputAssembly>([](GLPipeline::InputAssembly& ia) {
        ia.with_primitive(GLPrimitive::TriangleFan)
          .with_restart_index(0xFFFF);
    })
    .add<GLPipeline::DepthStencil>([](GLPipeline::DepthStencil& ds) {
        ds.no_depth_test();
    })
    .add<GLPipeline::Blend>([](GLPipeline::Blend& b) {
        b.alpha_blend();
    });

  pipeline.use();

  // Bind the VAO and (optionally) the IndexBuffer as late
  //   as possible and in the correct order (can't bind to
  //   ELEMENT_ARRAY_BUFFER with no VAO bound)
  verts->bind();
  if(inds) inds->bind();

  switch(command) {
  case DrawArray:
    pipeline.draw(count, offset);
    //glDrawArrays(GL_TRIANGLE_FAN, offset, count);
    break;

  case DrawIndexed:
    pipeline.drawIndexed(count, offset);
    //glDrawElements(GL_TRIANGLE_FAN, count, gl_inds_type, offset_ptr);
    break;

  case DrawArrayInstanced:
    pipeline.draw(count, offset, instance_count);
    //glDrawArraysInstanced(GL_TRIANGLE_FAN, offset, count, instance_count);
    break;

  case DrawIndexedInstanced:
    pipeline.drawIndexed(count, offset, instance_count);
    //glDrawElementsInstanced(
    //    GL_TRIANGLE_FAN, count, gl_inds_type, offset_ptr, instance_count
    //);
    break;

  default: assert(0);   // Unreachable
  }

  // Make sure to clean-up after drawing as leaving VAOs and 'ELEMENT_ARRAY_BUFFER'
  //   buffer objects bound can alter these object's internal state unexpectedly
  verts->unbind();
  if(inds) inds->unbind();

  return std::move(GLFence().fence());   // Return a primed fence
}

}
