#pragma once

#include <types.h>
#include <gx/context.h>

namespace brdrive {

// Forward declaration
class IWindow;

// PIMPL struct
struct pGLXContext;

class GLXContext : public GLContext {
public:
  GLXContext();
  virtual ~GLXContext();

  virtual auto acquire(
      IWindow *window, GLContext *share = nullptr
    ) -> GLContext&;

  virtual auto makeCurrent() -> GLContext&;
  virtual auto swapBuffers() -> GLContext&;
  virtual auto destroy() -> GLContext&;

  virtual auto handle() -> GLContextHandle;

private:
  pGLXContext *p;
};

}
