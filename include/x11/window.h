#pragma once

#include <x11/x11.h>
#include <window/window.h>

#include <exception>
#include <stdexcept>
#include <string>

namespace brdrive {

using X11WindowHandle = u32;

// Forward declarations
class X11EventLoop;
class GLXContext;

// PIMPL class
struct pX11Window;

class X11Window : public IWindow {
public:
  struct X11InternalError : public std::runtime_error {
    X11InternalError() :
      std::runtime_error("an error occured during communication with the X server")
    { }
  };

  struct X11NoSuchFontError : public std::runtime_error {
    X11NoSuchFontError() :
      std::runtime_error("the font could not be found!")
    { }
  };

  X11Window();
  X11Window(const X11Window&) = delete;
  X11Window(X11Window&&) = delete;
  virtual ~X11Window();

  virtual auto create() -> IWindow&;
  virtual auto show() -> IWindow&;

  virtual auto destroy() -> IWindow&;

  virtual auto drawString(const std::string& str, const Geometry& geom, const Color& color,
      const std::string& font = "") -> IWindow&;

  auto windowHandle() -> X11WindowHandle;

private:
  friend X11EventLoop;
  friend GLXContext;

  // Used by GLXContext to make sure the window has
  //   the visual required by the chosen FBConfig
  auto recreateWithVisualId(u8 depth, u32 visual_id) -> bool;

  pX11Window *p;
};

}
