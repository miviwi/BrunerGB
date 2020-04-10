#include <osd/drawcall.h>
#include <osd/surface.h>

#include <gx/context.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/buffer.h>
#include <gx/texture.h>
#include <gx/fence.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

#include <utility>
#include <limits>

namespace brdrive {

OSDDrawCall::OSDDrawCall() :
  command(DrawInvalid), type(DrawTypeInvalid),
  verts(nullptr), inds_type(GLType::Invalid),
  offset(-1), count(-1), instance_count(-1),
  textures_end(0)
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
  auto tex_and_sampler = [](
      GLTexture *tex, GLSampler *sampler
    ) -> OSDDrawCall::TextureAndSampler
  {
    return OSDDrawCall::TextureAndSampler(tex, sampler);
  };

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

  drawcall.textures_end = 3; // 2 Textures sequentially - thus index '2' is one past the last one

  return drawcall;
}

auto osd_submit_drawcall(
    GLContext& gl_context,  const OSDDrawCall& drawcall
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

auto OSDDrawCall::submit(SubmitFriendKey, GLContext& gl_context) const -> GLFence
{
  assert((command != DrawInvalid && type != DrawTypeInvalid) &&
      "attempted to submit an invalid OSDDrawCall!");

  // Bind all the required textures and upload the TexImageUnit
  //   uniforms to the OSDSurface::renderProgram(type)...
  auto& program = OSDSurface::renderProgram(type);
  program
    .use();

  switch(type) {
  case OSDDrawCall::DrawString:
    program.uniform("uiStringAttributesBaseOffset", (int)base_instance);
    break;
  }

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

  auto gl_inds_type = GLType_to_index_buf_type(inds_type);
  auto offset_ptr = (const GLvoid *)offset;

  assert((command != DrawIndexed && command != DrawIndexedInstanced) ||
        (gl_inds_type != GL_INVALID_ENUM) &&
      "an invalid type was given for the drawcall's index buffer elements!");

  // Bind the VAO and (optionally) the IndexBuffer as late
  //   as possible and in the correct order (can't bind to
  //   ELEMENT_ARRAY_BUFFER with no VAO bound)
  verts->bind();
  if(inds) inds->bind();

  switch(command) {
  case DrawArray:
    glDrawArrays(GL_TRIANGLE_FAN, offset, count);
    break;

  case DrawIndexed:
    glDrawElements(GL_TRIANGLE_FAN, count, gl_inds_type, offset_ptr);
    break;

  case DrawArrayInstanced:
    glDrawArraysInstanced(GL_TRIANGLE_FAN, offset, count, instance_count);
    break;

  case DrawIndexedInstanced:
    glDrawElementsInstanced(
        GL_TRIANGLE_FAN, count, gl_inds_type, offset_ptr, instance_count
    );
    break;

  default: assert(0);   // Unreachable
  }

  // Make sure to clean-up after drawing as leaving VAOs and 'ELEMENT_ARRAY_BUFFER'
  //   buffer objects bound can alter those object's internal state unexpectedly
  verts->unbind();
  if(inds) inds->unbind();

  return std::move(GLFence().fence());   // Return a primed fence
}

}
