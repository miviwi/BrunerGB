#pragma once

#include <gx/gx.h>

#include <exception>
#include <stdexcept>
#include <array>

namespace brdrive {

// Forward declarations
class IWindow;

class GLTexImageUnit;
class GLBufferBindPoint;
class GLTexture2D;
class GLSampler;

enum GLBufferBindPointType : unsigned;
// --------------------

// Handle to the underlying OS-specific OpenGL context structure
using GLContextHandle = void *;

struct GLVersion {
  int major, minor;
};

class GLContext {
public:
  struct NoSuitableFramebufferConfigError : public std::runtime_error {
    NoSuitableFramebufferConfigError() :
      std::runtime_error("no siutable frmebuffer config could be found!")
    { }
  };

  struct AcquireError : public std::runtime_error {
    AcquireError() :
      std::runtime_error("failed to acquire the GLContext!")
    { }
  };

  struct MakeCurrentError : public std::runtime_error {
    MakeCurrentError() :
      std::runtime_error("failed to make the GLContext the current context!")
    { }
  };

  struct NotADebugContextError : public std::runtime_error {
    NotADebugContextError() :
      std::runtime_error("the operation can only be performed on a debug OpenGL context!")
    { }
  };

  GLContext();
  GLContext(const GLContext&) = delete;
  virtual ~GLContext();

  static GLContext *current();

  // Acquires and initializes the GLContext
  virtual auto acquire(
      IWindow *window, GLContext *share = nullptr
    ) -> GLContext& = 0;

  // Makes this GLContext the 'current' context
  virtual auto makeCurrent() -> GLContext& = 0;

  // Swaps the front and back buffers
  virtual auto swapBuffers() -> GLContext& = 0;

  // Destroys the GLContext, which means
  //   acquire() can be called on it again
  virtual auto destroy() -> GLContext& = 0;

  // Returns a handle to the underlying OS-specific
  //   OpenGL context or nullptr if acquire() hasn't
  //   yet been called
  virtual auto handle() -> GLContextHandle = 0;

  auto texImageUnit(unsigned slot) -> GLTexImageUnit&;

  // If ARB::direct_state_access || EXT::direct_state_access
  //   this method always returns 0
  auto activeTexture() const -> unsigned;

  auto bufferBindPoint(
      GLBufferBindPointType bind_point, unsigned index
    ) -> GLBufferBindPoint&;

  // Can only be called AFTER gx_init()!
  auto dbg_EnableMessages() -> GLContext&;

  auto dbg_PushCallGroup(const char *name) -> GLContext&;
  auto dbg_PopCallGroup() -> GLContext&;

  auto versionString() -> std::string;
  auto version() -> GLVersion;

protected:
  // MUST be called by derived makeCurrent() right before they return!
  void postMakeCurrentHook();

  bool was_acquired_;

private:
  friend GLTexImageUnit;
  friend GLBufferBindPoint;

  GLTexImageUnit *tex_image_units_;        //   Array
  unsigned active_texture_;

  GLBufferBindPoint *buffer_bind_points_;  // ---||---

  unsigned dbg_group_id_;
};

}
