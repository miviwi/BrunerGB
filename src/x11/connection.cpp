#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_event.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <unordered_map>

namespace brgb {

static const char X11_WM_PROTOCOLS[]     = "WM_PROTOCOLS";
static const char X11_WM_DELETE_WINDOW[] = "WM_DELETE_WINDOW";
static const char X11_WM_WINDOW_ROLE[]   = "WM_WINDOW_ROLE";

struct pX11Connection {
  Display *xlib_display = nullptr;
  int default_screen    = -1;

  xcb_connection_t *connection = nullptr;
  const xcb_setup_t *setup     = nullptr;
  xcb_screen_t *screen         = nullptr;

  xcb_atom_t atom_wm_protocols     = X11InvalidId;
  xcb_atom_t atom_wm_delete_window = X11InvalidId;
  xcb_atom_t atom_wm_window_role   = X11InvalidId;

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

  // Intern all the xcb_atom_t's whih will be used later
  auto atom_wm_protocols_cookie = xcb_intern_atom(
      connection, 1 /* only_if_exists */,
      sizeof(X11_WM_PROTOCOLS)-1 /* don't include '\0' */, X11_WM_PROTOCOLS
  );
  auto atom_wm_delete_window_cookie = xcb_intern_atom(
      connection, 1 /* only_if_exists */,
      sizeof(X11_WM_DELETE_WINDOW)-1 /* don't include '\0' */, X11_WM_DELETE_WINDOW
  );
  auto atom_wm_window_role_cookie = xcb_intern_atom(
      connection, 1 /* only_if_exists */,
      sizeof(X11_WM_WINDOW_ROLE)-1 /* don't include '\0' */, X11_WM_WINDOW_ROLE
  );

  xcb_generic_error_t *intern_wm_protocols_err     = nullptr;
  xcb_generic_error_t *intern_wm_delete_window_err = nullptr;
  xcb_generic_error_t *intern_wm_window_role_err   = nullptr;

  auto atom_wm_protocols_reply = xcb_intern_atom_reply(
      connection, atom_wm_protocols_cookie, &intern_wm_protocols_err
  );
  auto atom_wm_delete_window_reply = xcb_intern_atom_reply(
      connection, atom_wm_delete_window_cookie, &intern_wm_delete_window_err
  );
  auto atom_wm_window_role_reply = xcb_intern_atom_reply(
      connection, atom_wm_window_role_cookie, &intern_wm_window_role_err
  );

  // Check for errors
  if(intern_wm_protocols_err || intern_wm_delete_window_err
      || intern_wm_window_role_err) {

    free(intern_wm_protocols_err);
    free(intern_wm_delete_window_err);

    free(intern_wm_window_role_err);

    return false;
  }

  // Store the xcb_atom_t's
  atom_wm_protocols = atom_wm_protocols_reply->atom;
  atom_wm_delete_window = atom_wm_delete_window_reply->atom;

  atom_wm_window_role = atom_wm_window_role_reply->atom;

  // <leanup
  free(atom_wm_protocols_reply);
  free(atom_wm_delete_window_reply);

  free(atom_wm_window_role_reply);

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

auto X11Connection::atom(X11AtomName name) -> X11Id
{
  assert(p && "must be called AFTER connect()!");

  switch(name) {
  case X11Atom_WM_Protocols:    return p->atom_wm_protocols;
  case X11Atom_WM_DeleteWindow: return p->atom_wm_delete_window;
  case X11Atom_WM_WindowRole:   return p->atom_wm_window_role;
  }

  return X11InvalidId;
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
