#pragma once

#include <x11/x11.h>

#include <exception>
#include <stdexcept>

namespace brdrive {

using X11ConnectionHandle = void /* xcb_connection_t */ *;
using X11SetupHandle      = const void /* xcb_setup_t */ *;
using X11ScreenHandle     = void /* xcb_screen_t */ *;
using X11KeyCode          = u8;

using X11XlibDisplayHandle = void /* Display */ *;

// PIMPL struct
struct pX11Connection;

class X11Connection {
public:
  struct X11ConnectError : std::runtime_error {
    X11ConnectError() :
      std::runtime_error("failed to connect to the X server!")
    { }
  };

  X11Connection();
  X11Connection(const X11Connection&) = delete;
  ~X11Connection();

  auto connect() -> X11Connection&;

  // Returns a xcb_connection_t* as an X11ConnectionHandle
  auto connectionHandle() -> X11ConnectionHandle;
  // Returns a const xcb_setup_t* as an X11SetupHandle
  auto setupHandle() -> X11SetupHandle;
  // Returns a const xcb_sscreen_t* as an X11screenHandle
  auto screenHandle() -> X11ScreenHandle;
  // Returns an Xlib Display* as an X11XlibDisplayHandle
  auto xlibDisplayHandle() -> X11XlibDisplayHandle;

  auto defaultScreen() -> int;

  // T must always be xcb_connection_t
  template <typename T>
  auto connection() -> T*
  {
    return (T *)connectionHandle();
  }

  // T must always be xcb_setup_t
  template <typename T>
  auto setup() -> const T*
  {
    return (const T *)setupHandle();
  }

  // T must always be xcb_screen_t
  template <typename T>
  auto screen() -> T*
  {
    return (T *)screenHandle();
  }

  // T must always be Display
  template <typename T>
  auto xlibDisplay() -> T*
  {
    return (T *)xlibDisplayHandle();
  }

  // Returns a new XId siutable for assigning
  //   to new graphics contexts, windows etc.
  auto genId() -> u32;

  // Calls xcb_flush(connection())
  auto flush() -> X11Connection&;

  auto keycodeToKeysym(X11KeyCode keycode) -> u32;

private:
  pX11Connection *p;
};

}
