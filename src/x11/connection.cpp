#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <unordered_map>

namespace brdrive {

struct pX11Connection {
  Display *xlib_display;
  int default_screen;

  xcb_connection_t *connection;
  const xcb_setup_t *setup;
  xcb_screen_t *screen;

  std::unordered_map<xcb_keycode_t, u32> keycode_to_keysym;

  ~pX11Connection();

  auto keycodeToKeysym(xcb_keycode_t keycode) const -> int
  {
    return keycode_to_keysym.at(keycode);
  }

  auto connect() -> bool;

  auto initKbmap() -> bool;
};

pX11Connection::~pX11Connection()
{
  xcb_disconnect(connection);
}

auto pX11Connection::connect() -> bool
{
  // Need to connect through Xlib to use GLX
  xlib_display = XOpenDisplay(nullptr);
  if(!xlib_display) return false;

  default_screen = DefaultScreen(xlib_display);

  // Create the xcb connection
  connection = XGetXCBConnection(xlib_display);
  if(!connection) return false;

  // Acquire event queue ownership
  XSetEventQueueOwner(xlib_display, XCBOwnsEventQueue);

  setup = xcb_get_setup(connection);
  if(!setup) return false;

  // Iterate to find the default screen...
  auto roots_it = xcb_setup_roots_iterator(setup);
  while(roots_it.index < default_screen) {
    // Make sure we don't advance past the end
    if(!roots_it.rem) return false;

    xcb_screen_next(&roots_it);
  }

  // ...and save it
  screen = roots_it.data;

  return true;
}

auto pX11Connection::initKbmap() -> bool
{
  auto kbmap_cookie = xcb_get_keyboard_mapping(
      connection,
      setup->min_keycode, setup->max_keycode-setup->min_keycode + 1
  );
  auto kbmap = xcb_get_keyboard_mapping_reply(
      connection, kbmap_cookie, nullptr
  );
  if(!kbmap) return false;

  auto num_keysyms = kbmap->length;
  auto keysyms_per_keycode = kbmap->keysyms_per_keycode;
  auto num_keycodes = num_keysyms / keysyms_per_keycode;

  auto keysyms = (xcb_keysym_t *)(kbmap + 1);

  for(int keycode_idx = 0; keycode_idx < num_keycodes; keycode_idx++) {
    auto keycode = setup->min_keycode + keycode_idx;
    auto lowercase_keysym_idx = keycode_idx*keysyms_per_keycode + 0;

    keycode_to_keysym.emplace(keycode, keysyms[lowercase_keysym_idx]);
  }

  free(kbmap);

  return true;
}

X11Connection::X11Connection() :
  p(nullptr)
{
}

X11Connection::~X11Connection()
{
  delete p;
}

auto X11Connection::connect() -> X11Connection&
{
  p = new pX11Connection();

  // Establish a connection to the X server
  if(!p->connect()) throw X11ConnectError();

  // Cache commonly used data
  if(!p->initKbmap()) throw X11ConnectError();

  return *this;
}

auto X11Connection::connectionHandle() -> X11ConnectionHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->connection;
}

auto X11Connection::setupHandle() -> X11SetupHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->setup;
}

auto X11Connection::screenHandle() -> X11ScreenHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->screen;
}

auto X11Connection::xlibDisplayHandle() -> X11XlibDisplayHandle
{
  assert(p && "must be called AFTER connect()!");

  return p->xlib_display;
}

auto X11Connection::defaultScreen() -> int
{
  assert(p && "must be called AFTER connect()!");

  return p->default_screen;
}

auto X11Connection::genId() -> u32
{
  return xcb_generate_id(p->connection);
}

auto X11Connection::flush() -> X11Connection&
{
  xcb_flush(p->connection);

  return *this;
}

auto X11Connection::keycodeToKeysym(X11KeyCode keycode) -> u32
{
  assert(p && "must be called AFTER connect()!");

  return p->keycodeToKeysym(keycode);
}

}
