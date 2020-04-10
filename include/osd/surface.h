#pragma once

#include <osd/osd.h>
#include <osd/util.h>

#include <window/geometry.h>
#include <window/color.h>
#include <gx/handle.h>

#include <exception>
#include <stdexcept>
#include <vector>
#include <memory>

namespace brdrive {

// Forward declarations
class OSDBitmapFont;
class OSDDrawCall;
class GLVertexArray;
class GLProgram;
class GLTexture;
class GLTexture2D;
class GLTextureBuffer;
class GLSampler;
class GLBuffer;
class GLVertexBuffer;
class GLBufferTexture;
class GLIndexBuffer;
class GLPixelBuffer;

// PIMPL struct
struct pOSDSurface;

class OSDSurface {
public:
  struct NullSurfaceError : public std::runtime_error {
    NullSurfaceError() :
      std::runtime_error("create() wasn't called!")
    { }
  };

  struct FontNotProvidedError : public std::runtime_error {
    FontNotProvidedError() :
      std::runtime_error("a font wasn't provided to create()")
    { }
  };

  OSDSurface();
  ~OSDSurface();

  //  - 'font' must be valid only until the return of
  //    this function call
  auto create(
      ivec2 width_height, const OSDBitmapFont *font = nullptr,
      const Color& bg = Color::transparent()
    ) -> OSDSurface&;

  auto writeString(ivec2 pos, const char *string, const Color& color) -> OSDSurface&;

  auto draw() -> std::vector<OSDDrawCall>;

  // Clears any objects written to the surface up to
  //   this point i.e. after this call draw() is gua-
  //   -ranteed to return an empty vector
  auto clear() -> OSDSurface&;

  static auto renderProgram(int /* OSDDrawCall::DrawType */ draw_type) -> GLProgram&;

private:
  // Functions that manage static class member creation/destruction
  friend void osd_init();
  friend void osd_finalize();

  enum {
    SurfaceVertexBufSize = 4 * 1024,
    SurfaceIndexBufSize  = 4 * 1024,

    StringsGPUBufSize     = 256 * 1024, // 256KiB
    StringAttrsGPUBufSize = 4 * 1024,   // 4KiB
  };

  void initGLObjects();

  // Always called by initGLObjects()
  void initCommonGLObjects();

  // Called by initGLObjects()
  //   - Only if create() was called with a font
  void initFontGLObjects();

  void destroyGLObjects();

  void destroyCommonGLObjects();
  void destroyFontGLObjects();

  void appendStringDrawcalls(std::vector<OSDDrawCall>& drawcalls);

  // Array of GLProgram *[OSDDrawCall::NumDrawTypes]
  //   - NOTE: pointers in this array CAN be nullptr
  //      (done for ease of indexing)
  static GLProgram **s_surface_programs;

  ivec2 dimensions_;
  const OSDBitmapFont *font_;
  Color bg_;

  // Set to 'true' after create() is called
  bool created_;

  // String related data structures
  struct StringObject {
    ivec2 position;
    std::string str;
    Color color;
  };

  std::vector<StringObject> string_objects_;

  mat4 m_projection;

  // Generic gx objects (used for drawing everything)
  GLVertexArrayHandle empty_vertex_array_;

  //   * attached to 'empty_vertex_array_'
  GLVertexBuffer *surface_object_verts_;
  GLIndexBuffer *surface_object_inds_;

  // String-related gx objects
  GLTexture2D *font_tex_;
  GLSampler *font_sampler_;

  //  * string data (i.e. the strings themselves)
  GLBufferTexture *strings_buf_;
  GLTextureBuffer *strings_tex_;

  //  * string attributes:
  //      position, offset in 'strings_buf_', size, color
  GLBufferTexture *string_attrs_buf_;
  GLTextureBuffer *string_attrs_tex_;
};

}
