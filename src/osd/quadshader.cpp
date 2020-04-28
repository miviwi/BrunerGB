#include "osd/drawcall.h"
#include <osd/quadshader.h>
#include <osd/shaders.h>

#include <gx/program.h>
#include <util/format.h>

#include <utility>
#include <sstream>

#include <cassert>

namespace brgb {

static unsigned p_next_quadshader_id = 0;

static const char *p_quadshader_fs_prelude = R"FRAG(
in Vertex {
  vec2 UV;
} fi;

#if defined(NO_BLEND)
#  define OUTPUT_CHANNELS vec3
#else
#  define OUTPUT_CHANNELS vec4
#endif

out OUTPUT_CHANNELS foFragColor;
)FRAG";

OSDQuadShader::OSDQuadShader(OSDQuadShader&& other) :
  OSDQuadShader()
{
  std::swap(frozen_, other.frozen_);
  std::swap(source_, other.source_);
  std::swap(program_, other.program_);
}

OSDQuadShader::~OSDQuadShader()
{
  destroy();
}

auto OSDQuadShader::addSource(const char *src) -> OSDQuadShader&
{
  // See comment above OSDQuadShader::frozen() declaration
  if(frozen()) throw AddToFrozenShaderError();

  source_ += src;

  return *this;
}

auto OSDQuadShader::addSource(const std::string& src) -> OSDQuadShader&
{
  // See comment above OSDQuadShader::frozen() declaration
  if(frozen()) throw AddToFrozenShaderError();

  source_ += src;

  return *this;
}

auto OSDQuadShader::addPixmapArray(const char *name, size_t len) -> OSDQuadShader&
{
  pixmap_arrays_.emplace_back(name, len);

  return *this;
}

auto OSDQuadShader::declFunction(const char *signature) -> OSDQuadShader&
{
  function_decls_.emplace_back(signature);

  return *this;
}

auto OSDQuadShader::addFunction(const char *signature, const char *src) -> OSDQuadShader&
{
  declFunction(signature);
  addSource(util::fmt("%s\n{\n%s}\n", signature, src));

  return *this;
}

auto OSDQuadShader::entrypoint(const char *func_name) -> OSDQuadShader&
{
  entrypoint_ = func_name;

  return *this;
}

auto OSDQuadShader::program() -> GLProgram&
{
  if(program_) return *program_;    // Return the compiled program (if available)

  if(entrypoint_.empty()) throw EntrypointUndefinedError();

  auto vert_ptr = osd_detail::compile_DrawShadedQuad_vertex_shader();

  auto& vert = *vert_ptr;
  auto frag = GLShader(GLShader::Fragment);

  std::ostringstream pixmap_uniform_decls;

  pixmap_uniform_decls << '\n';
  for(const auto& [name, len] : pixmap_arrays_) {
    pixmap_uniform_decls << util::fmt("uniform sampler2D %s[%zu];\n", name, len);
  }
  pixmap_uniform_decls << '\n';

  std::ostringstream function_decls;

  function_decls << '\n';
  for(const auto& signature : function_decls_) {
    function_decls << util::fmt("%s;\n", signature.data());
  }
  function_decls << '\n';

  frag
    .source(p_quadshader_fs_prelude)
    .source(pixmap_uniform_decls.str())
    .source(function_decls.str())
    .source(util::fmt("\nvoid main() { foFragColor = %s(); }\n", entrypoint_))
    .source(source_)
    .compile();

  frag.label(util::fmt("p.OSD.QuadShader%uFS", p_next_quadshader_id).data());

  program_ = new GLProgram();
  auto& program = *program_;

  program
    .attach(vert)
    .attach(frag);

  program.label(util::fmt("p.OSD.QuadShader%u", p_next_quadshader_id).data());

  program
    .link()
    .detach(vert)
    .detach(frag);

  p_next_quadshader_id++;
  frozen_ = true;

  return program;
}

auto OSDQuadShader::frozen() -> bool
{
  return frozen_;
}

auto OSDQuadShader::destroy() -> void
{
  delete program_;
}

}
