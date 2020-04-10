#include <x11/glx.h>
#include <x11/x11.h>
#include <x11/connection.h>
#include <x11/window.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

// OpenGL headers
#include <GL/gl.h>

namespace brdrive {

static int GLX_VisualAttribs[] = {
  GLX_X_RENDERABLE, True,
  GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,

  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,

  GLX_RENDER_TYPE, GLX_RGBA_BIT,
  GLX_RED_SIZE,   8,
  GLX_GREEN_SIZE, 8,
  GLX_BLUE_SIZE,  8,
  GLX_ALPHA_SIZE, 8,

//  GLX_DEPTH_SIZE, 24,
//  GLX_STENCIL_SIZE, 8,

  GLX_DOUBLEBUFFER, True,

  None,
};

// glXCreateContextAttribsARB is an extension
//   so it has to be defined manually
using glXCreateContextAttribsARBFn = ::GLXContext (*)(
    Display *, GLXFBConfig, ::GLXContext, Bool, const int *
  );

static bool g_createcontextattribs_queried = false;
static glXCreateContextAttribsARBFn glXCreateContextAttribsARB = nullptr;

// ...Same with glXSwapIntervalEXT
using glXSwapIntervalEXTFn = void (*)(
    Display *dpy, GLXDrawable drawable, int interval
  );

static bool g_swapinterval_queried = false;
static glXSwapIntervalEXTFn glXSwapIntervalEXT = nullptr;

struct pGLXContext {
  Display *display;

  ::GLXContext context = nullptr;
  GLXWindow window = 0;

  ~pGLXContext();

  auto createContext(
      const GLXFBConfig& fb_config, IWindow *window, GLContext *share
    ) -> bool;
  auto createContextLegacy(
      const GLXFBConfig& fb_config, IWindow *window, GLContext *share
    ) -> bool;
};

pGLXContext::~pGLXContext()
{
  auto display = x11().xlibDisplay<Display>();

  if(window) glXDestroyWindow(display, window);
  if(context) glXDestroyContext(display, context);
}

auto pGLXContext::createContext(
    const GLXFBConfig& fb_config, IWindow *window, GLContext *share
  ) -> bool
{
  if(!g_createcontextattribs_queried) {
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBFn)
      glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
  }

  if(!g_swapinterval_queried) {
    glXSwapIntervalEXT = (glXSwapIntervalEXTFn)
      glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
  }

  // Only old-style contexts are available 
  if(!glXCreateContextAttribsARB) return false;

#if !defined(NDEBUG)
  int context_flags = GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#else
  int context_flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#endif

  const int context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,

    GLX_CONTEXT_FLAGS_ARB, context_flags,

    None,
  };

  context = glXCreateContextAttribsARB(
      display, fb_config, 
      /* shareList */ share ? (::GLXContext)share->handle() : nullptr,
      /* direct */ True,
      context_attribs
  );
  if(!context) return false;

  return true;
}

auto pGLXContext::createContextLegacy(
    const GLXFBConfig& fb_config, IWindow *window, GLContext *share
  ) -> bool
{
  context = glXCreateNewContext(
      display, fb_config, GLX_RGBA_TYPE,
      /* shareList */ share ? (::GLXContext)share->handle() : nullptr,
      /* direct */ True
  );

  return context;
}

GLXContext::GLXContext() :
  p(nullptr)
{
}

GLXContext::~GLXContext()
{
  delete p;
}

auto GLXContext::acquire(IWindow *window_, GLContext *share) -> GLContext&
{
  assert(brdrive::x11_was_init() &&
      "x11_init() MUST be called prior to creating a GLXContext!");

  auto display = x11().xlibDisplay<Display>();

  int num_fb_configs = 0;
  auto fb_configs = glXChooseFBConfig(
      display, x11().defaultScreen(),
      GLX_VisualAttribs, &num_fb_configs
  );
  if(!fb_configs || !num_fb_configs) throw NoSuitableFramebufferConfigError();

  // TODO: smarter way of choosing the FBConfig (?)
  auto fb_config = fb_configs[0];
  XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fb_config);

  auto cleanup_x_structures = [&]() -> void {
    XFree(visual_info);
    XFree(fb_configs);
  };

  auto depth  = visual_info->depth;
  auto visual = visual_info->visualid;

  // The visual of the window must match that of the
  //   FBConfig, so make sure that's the case before
  //   assigning the context to it
  auto window = (X11Window *)window_;
  if(!window->recreateWithVisualId(depth, visual)) {
    cleanup_x_structures();

    throw X11Window::X11InternalError();
  }

  // The XID of the window
  auto x11_window_handle = window->windowHandle();

  p = new pGLXContext();

  p->display = display;

  // Try to create a new-style context first...
  if(!p->createContext(fb_config, window, share)) {
    // ...and fallback to old-style glXCreateNewContext
    //    if it fails
    p->createContextLegacy(fb_config, window, share);
  }

  // Check if context creation was successful
  if(!p->context) throw AcquireError();

  p->window = glXCreateWindow(
      display, fb_config, x11_window_handle, nullptr
  );
  if(!p->window) {
    glXDestroyContext(display, p->context);
    cleanup_x_structures();

    delete p;
    p = nullptr;

    throw AcquireError();
  }

  glXSwapIntervalEXT(display, p->window, 1);

  cleanup_x_structures();

  // Mark the context as successfully acquired
  was_acquired_ = true;

  return *this;
}

auto GLXContext::makeCurrent() -> GLContext&
{
  assert(was_acquired_ && "the context must've been acquire()'d to makeCurrent()!");

  auto display  = p->display;
  auto drawable = (GLXDrawable)p->window;
  auto context  = p->context;

  auto success = glXMakeContextCurrent(display, drawable, drawable, context);
  if(!success) throw AcquireError();

  postMakeCurrentHook();

  return *this;
}

auto GLXContext::swapBuffers() -> GLContext&
{
  assert(was_acquired_ && "the context must've been acquire()'d to swapBuffers()!");

  auto drawable = (GLXDrawable)p->window;
  glXSwapBuffers(p->display, drawable);

  return *this;
}

auto GLXContext::destroy() -> GLContext&
{
  if(!p) return *this;

  delete p;
  p = nullptr;

  return *this;
}

auto GLXContext::handle() -> GLContextHandle
{
  if(!p) return nullptr;

  return p->context;
}

}

