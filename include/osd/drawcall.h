#pragma once

#include <osd/osd.h>

#include <gx/gx.h>

#include <tuple>

namespace brdrive {

// Forward declaractions
class OSDBitmapFont;
class GLContext;
class GLVertexArray;
class GLProgram;
class GLTexture;
class GLTexture2D;
class GLTextureBuffer;
class GLSampler;
class GLBuffer;
class GLIndexBuffer;
class GLBufferTexture;
class GLPixelBuffer;
class GLFence;

// Any pointers stored in this object
//   will NOT be freed by it i.e. it is
//   the responsibility of the CALLER
struct OSDDrawCall {
  enum DrawCommandType {
    DrawInvalid,

    DrawArray,
    DrawIndexed,
    DrawArrayInstanced,
    DrawIndexedInstanced,
  };

  enum DrawType : int {
    DrawTypeInvalid,

    DrawString,
    DrawRectangle,
    DrawShadedQuad,

    NumDrawTypes,
  };

  OSDDrawCall();

  DrawCommandType command;
  DrawType type;

  GLVertexArray *verts;

  GLType inds_type;
  GLIndexBuffer *inds;

  GLSizePtr offset;
  GLSize count;
  GLSize base_instance;
  GLSize instance_count;

  using TextureAndSampler = std::tuple<GLTexture *, GLSampler *>;
  using TextureBindings = std::array<TextureAndSampler, GLNumTexImageUnits>;

  TextureBindings textures;
  // 1 past the offset after which no more entries are present in 'textures'
  size_t textures_end;

/*
semi-private:
*/
  class SubmitFriendKey {
    SubmitFriendKey() { }

    friend auto osd_submit_drawcall(
        GLContext& gl_context, const OSDDrawCall& drawcall
      ) -> GLFence;
  };

  auto submit(SubmitFriendKey, GLContext& gl_context) const -> GLFence;
};

// - 'verts_' should contain an ivec4 attribute at location 0, where:
//        attr.xy is this string's position relative to the top-left corner
//          expressed in screen pixels
//        attr.z is the 0-based offset at which this string's data starts
//          in the 'strings_' texture buffer
//        attr.w is the length of the string expressed in characters
// - The index Buffer MUST contain 5 vertices per glyph formed in this fasion:
//       n+0, n+1, n+2, n+3, 0xFFFF, n+4, n+5, n+6, n+7, 0xFFFF...
// - The max_string_len_ must be the maximum length of all the strings residing
//   in the strings_ texture buffer, given in number of characters
// - The 'font_sampler_' is optional and can be set to 'nullptr'
// - 'strings_' should contain successive characters of tightly packed strings ex.
//       auto string1 = "hello";
//       auto string2 = "John Doe!";
//
//       strings_  =>  "helloJohn Doe!"
//   * Example 'verts_' buffer data which fits the above 'strings_' buffer
//       i16[] = {
//         // instance[0]  =>  string1
//         20 /* screen_x */, 10 /* screen_y */, 0 /* offset */, sizeof(string1)-1, /* size */
//
//         // instance[1]  =>  string2
//         20, 100, sizeof(string1) /* comes right after string1 */, sizeof(string2)-1,
//       };
auto osd_drawcall_strings(
    GLVertexArray *verts_, GLType inds_type_, GLIndexBuffer *inds_, GLSizePtr base_offset,
    GLSize max_string_len_, GLSize num_strings_,
    GLTexture2D *font_tex_, GLSampler *font_sampler_, GLTextureBuffer *strings_, GLTextureBuffer *attrs_
  ) -> OSDDrawCall;

// Sets up the proper state and calls glDraw<Arrays,Elements>[Instanced]()
//   according to the provided 'drawcall'
auto osd_submit_drawcall(
    GLContext& gl_context,  const OSDDrawCall& drawcall
  ) -> GLFence;

}
