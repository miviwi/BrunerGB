#include <osd/quadshader.h>
#include <osd/shaders.h>

#include <gx/program.h>
#include <util/format.h>

#include <cassert>

namespace brgb {

static unsigned p_next_quadshader_id = 0;

OSDQuadShader::~OSDQuadShader()
{
  destroy();
}

auto OSDQuadShader::addSource(const char *src) -> OSDQuadShader&
{
  // See comment above OSDQuadShader::frozen() declaration
  if(frozen_) throw AddToFrozenShaderError();

  source_ += src;

  return *this;
}

auto OSDQuadShader::addSource(const std::string& src) -> OSDQuadShader&
{
  // See comment above OSDQuadShader::frozen() declaration
  if(frozen_) throw AddToFrozenShaderError();

  source_ += src;

  return *this;
}

auto OSDQuadShader::program() -> GLProgram&
{
  auto vert_ptr = osd_detail::compile_DrawShadedQuad_vertex_shader();

  auto& vert = *vert_ptr;
  auto frag = GLShader(GLShader::Fragment);

  frag
    .source(source_)
    .compile();

  frag.label(util::fmt("p.OSD.QuadShaderFS%u", p_next_quadshader_id).data());

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

  return *program_;
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
