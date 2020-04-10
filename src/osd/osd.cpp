#include <osd/osd.h>
#include <osd/drawcall.h>
#include <osd/shaders.h>
#include <osd/surface.h>
#include <gx/program.h>

#include <cstdlib>
#include <cstdio>

#include <exception>

namespace brdrive {

static bool g_osd_was_init = false;

void osd_init()
{
  using namespace osd_detail;   // Reduce syntax noise...

  OSDSurface::s_surface_programs = new GLProgram *[OSDDrawCall::NumDrawTypes];

  // OSDDrawCall::DrawTypeInvalid  -  has no program, thus should never be used
  OSDSurface::s_surface_programs[OSDDrawCall::DrawInvalid] = nullptr;

  // OSDDrawCall::DrawString
  OSDSurface::s_surface_programs[OSDDrawCall::DrawString] = init_DrawString_program();

  // OSDDrawCall::DrawRectangle
  //   TODO: unimplemented...
  OSDSurface::s_surface_programs[OSDDrawCall::DrawRectangle] = init_DrawRectangle_program();

  // OSDDrawCall::DrawShadedQuad
  //   TODO: unimplemented...
  OSDSurface::s_surface_programs[OSDDrawCall::DrawShadedQuad] = init_DrawShadedQuad_program();

  g_osd_was_init = true;
}

void osd_finalize()
{
  for(size_t i = 0; i < OSDDrawCall::NumDrawTypes; i++) {
    auto program = OSDSurface::s_surface_programs[i];
    if(!program) continue;

    delete program;
  }
  delete[] OSDSurface::s_surface_programs;

  // Make sure not to leave dangling pointers around
  OSDSurface::s_surface_programs = nullptr;

  g_osd_was_init = false;
}

auto osd_was_init() -> bool
{
  return g_osd_was_init;
}

}
