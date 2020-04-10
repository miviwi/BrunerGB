#include <gx/gx.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

namespace brdrive {

bool g_gx_was_init = false;
GLId g_null_vao = GLNullId;

void gx_init()
{
  auto result = gl3wInit();
  if(result != GL3W_OK) {
    printf("gl3wInit(): %d\n", result);

    throw GL3WInitError();
  }

  glCreateVertexArrays(1, &g_null_vao);
  glBindVertexArray(g_null_vao);
  glObjectLabel(GL_VERTEX_ARRAY, g_null_vao, -1, "a.Global.Null");

  assert(glGetError() == GL_NO_ERROR);

  g_gx_was_init = true;
}

void gx_finalize()
{
  glBindVertexArray(0);
  glDeleteVertexArrays(1, &g_null_vao);

  g_gx_was_init = false;
}

auto gx_was_init() -> bool
{
  return g_gx_was_init;
}

}
