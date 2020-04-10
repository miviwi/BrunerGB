#include <gx/context.h>
#include <gx/texture.h>
#include <gx/buffer.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>

#include <new>

namespace brdrive {

thread_local GLContext *g_current_context = nullptr;

static void dbg_message_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam
  )
{
  // Discard debug group messages
  if(source == GL_DEBUG_SOURCE_APPLICATION) return;

  auto prefix = type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "";

  fprintf(stderr,
    "OpenGL: %s type = 0x%x, severity = 0x%x, message = %s\n",
    prefix, type, severity, message
  );
}

GLContext::GLContext() :
  was_acquired_(false),
  tex_image_units_(nullptr),
  active_texture_(0),
  buffer_bind_points_(nullptr),
  dbg_group_id_(1)
{
  // Allocate backing memory via malloc() because GLTexImageUnit's constructor requires
  //   an argument and new[] doesn't support passing per-instance args
  tex_image_units_ = (GLTexImageUnit *)malloc(GLNumTexImageUnits * sizeof(GLTexImageUnit));
  for(unsigned i = 0; i < GLNumTexImageUnits; i++) {
    auto unit = tex_image_units_ + i;

    // Use placement-new to finalize creation of the object
    new(unit) GLTexImageUnit(this, i);
  }

  // Do the same for all the GLBufferBindPoints
  buffer_bind_points_ = (GLBufferBindPoint *)malloc(
      GLNumBufferBindPoints * GLBufferBindPointType::NumTypes * sizeof(GLBufferBindPoint)
  );
  for(size_t type_ = 0; type_ < GLBufferBindPointType::NumTypes; type_++) {
    auto type = (GLBufferBindPointType)type_;
    auto bind_points = buffer_bind_points_ + type_*GLNumBufferBindPoints;

    for(unsigned i = 0; i < GLNumBufferBindPoints; i++) {
      auto bind_point = bind_points + i;

      new(bind_point) GLBufferBindPoint(this, type, i);
    }
  }
}

GLContext::~GLContext()
{
  // Because malloc() was used to allocate 'tex_image_units_' we need to call
  //   the destructors on each of the GLTexImageUnits manually...
  for(unsigned i = 0; i < GLNumTexImageUnits; i++) {
    auto unit = tex_image_units_ + i;
    unit->~GLTexImageUnit();
  }

  // ...and release the memory with free()
  free(tex_image_units_);

  // ...and do the same for all the GLBufferBindPoints
  for(unsigned i = 0; i < GLNumBufferBindPoints*GLBufferBindPointType::NumTypes; i++) {
    auto bind_point = buffer_bind_points_ + i;
    bind_point->~GLBufferBindPoint();
  }

  free(buffer_bind_points_);
}

auto GLContext::current() -> GLContext *
{
  return g_current_context;
}

auto GLContext::dbg_EnableMessages() -> GLContext&
{
#if !defined(NDEBUG)
  int context_flags = -1;
  glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);

  if(!(context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) throw NotADebugContextError();

  // Enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  glDebugMessageCallback(dbg_message_callback, nullptr);
#endif

  return *this;
}

auto GLContext::dbg_PushCallGroup(const char *name) -> GLContext&
{
#if !defined(NDEBUG)
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, dbg_group_id_, -1, name);
  dbg_group_id_++;
#endif

  return *this;
}

auto GLContext::dbg_PopCallGroup() -> GLContext&
{
#if !defined(NDEBUG)
  glPopDebugGroup();
#endif

  return *this;
}

auto GLContext::versionString() -> std::string
{
  assert(gx_was_init() && "gx_init() must be called before using this method!");

  return std::string((const char *)glGetString(GL_VERSION));
}

auto GLContext::version() -> GLVersion
{
  assert(gx_was_init() && "gx_init() must be called before using this method!");

  auto version_string = glGetString(GL_VERSION);

  GLVersion version;
  sscanf((const char *)version_string, "%d.%d", &version.major, &version.minor);

  return version;
}

auto GLContext::texImageUnit(unsigned slot) -> GLTexImageUnit&
{
  assert(was_acquired_ && "the context must've been acquire()'d to use it's texImageUnits!");
  assert(slot < GLNumTexImageUnits && "'slot' must be < GLNumTexImageUnits!");

  return tex_image_units_[slot];
}

auto GLContext::activeTexture() const -> unsigned
{
  return active_texture_;
}

auto GLContext::bufferBindPoint(
    GLBufferBindPointType bind_point, unsigned index
  ) -> GLBufferBindPoint&
{
  assert(was_acquired_ && "the context must've been acquire()'d to use it's texImageUnits!");
  assert((unsigned)bind_point < GLBufferBindPointType::NumTypes && "'bind_point' is invalid!");
  assert(index < GLNumBufferBindPoints && "'index' must be < GLNumBufferBindPoints!");

  return buffer_bind_points_[bind_point*GLNumBufferBindPoints + index];
}

void GLContext::postMakeCurrentHook()
{
  g_current_context = this;
}

}
