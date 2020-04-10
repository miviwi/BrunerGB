#include <x11/window.h>
#include <x11/connection.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <X11/keysymdef.h>

#include <cassert>
#include <cstdlib>

#include <array>
#include <unordered_map>
#include <optional>

namespace brdrive {

template <typename... Args>
auto gc_args(Args&&... args)
{
  return std::array<u32, sizeof...(Args)>{ args... };
}

struct pX11Window {
  xcb_connection_t *connection;
  const xcb_setup_t *setup;
  xcb_screen_t *screen;

  xcb_window_t window;
  xcb_colormap_t colormap;

  ~pX11Window();

  // Returns 'false' on failure
  auto init(const Geometry& geom, const Color& bg_color) -> bool;

  auto font(const std::string& font_name) -> std::optional<xcb_font_t>
  {
    auto font_it = fonts.find(font_name);
    if(font_it == fonts.end()) {
      // If the font wasn't found try to load it
      auto font = openFont(font_name);
      if(!font) return std::nullopt;   // Failed to open the font

      // Font was opened - add it to 'fonts'
      //   for later retrieval and return it
      fonts.emplace(font_name, font.value());
      return font;
    }

    return font_it->second;
  }

  template <size_t N>
  auto createGc(u32 mask, std::array<u32, N> args) -> std::optional<xcb_gcontext_t>
  {
    auto gc = x11().genId();
    auto gc_cookie = xcb_create_gc_checked(
        connection, gc, window, mask, args.data()
    );
    auto err = xcb_request_check(connection, gc_cookie);
    if(!err) return gc;

    // Error - cleanup return std::nullopt
    free(err);
    return std::nullopt;
  }

  // Make SURE to free any existing windows/colormaps before
  //   calling the methods below!

  auto createColormap(xcb_visualid_t visual = 0) -> bool;

  auto createWindow(
      const Geometry& geom, const Color& bg_color,
      u8 depth = 0, xcb_visualid_t visual = 0
    ) -> bool;

private:
  auto openFont(const std::string& font_name) -> std::optional<xcb_font_t>;

  std::unordered_map<std::string, xcb_font_t> fonts;
};

pX11Window::~pX11Window()
{
  xcb_destroy_window(connection, window);
  xcb_free_colormap(connection, colormap);

  for(const auto& [name, font] : fonts) {
    xcb_close_font(connection, font);
  }
}

auto pX11Window::init(const Geometry& geom, const Color& bg_color) -> bool
{
  connection = x11().connection<xcb_connection_t>();
  setup = x11().setup<xcb_setup_t>();
  screen = x11().screen<xcb_screen_t>();

  window = screen->root;
  if(!createColormap()) return false;
  if(!createWindow(geom, bg_color)) return false;

  return true;
}

auto pX11Window::openFont(const std::string& font) -> std::optional<xcb_font_t>
{
  auto id = x11().genId();

  auto font_cookie = xcb_open_font_checked(
      connection, id, font.size(), font.data()
  );
  auto err = xcb_request_check(connection, font_cookie);
  if(!err) return id;   // The font was opened successfully

  // There was an error - cleanup and signal failure
  free(err);
  return std::nullopt;
}

auto pX11Window::createWindow(
    const Geometry& geom, const Color& bg_color,
    u8 depth, xcb_visualid_t visual
  ) -> bool
{
  // Acquire an id for the window
  window = x11().genId();

  u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  u32 args[] = {
    bg_color.bgr(),                       /* XCB_CW_BACK_PIXEL */
    XCB_EVENT_MASK_EXPOSURE               /* XCB_CW_EVENT_MASK */
    | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
    | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
    colormap,
  };
  auto window_cookie = xcb_create_window_checked(
      connection,
      visual ? depth : screen->root_depth, window, screen->root, geom.x, geom.y, geom.w, geom.h, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, visual ? visual : screen->root_visual, mask, args
  );
  auto err = xcb_request_check(connection, window_cookie);
  if(!err) {
#if !defined(NO_SET_WM_WINDOW_ROLE)
    static const char atom_wm_window_role[] = "WM_WINDOW_ROLE";
    auto atom_cookie = xcb_intern_atom(
        connection,
        0 /* only_if_exists */, sizeof(atom_wm_window_role)-1, atom_wm_window_role
    );

    xcb_generic_error_t *intern_atom_err = nullptr;
    auto atom_reply = xcb_intern_atom_reply(connection, atom_cookie, &intern_atom_err);
    if(intern_atom_err) {
      free(intern_atom_err);
      return false;
    }

    // Try to set the property - disergarding whether it succeedes or fails
    static const char role_popup[] = "pop-up";
    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE, window, atom_reply->atom, XCB_ATOM_STRING, 8,
        sizeof(role_popup)-1, role_popup
    );

    // Make sure the commands are sent to the X server
    xcb_flush(connection);

    free(atom_reply);
#endif

    return true;
  }

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

auto pX11Window::createColormap(xcb_visualid_t visual) -> bool
{
  // Acquire an id for the colormap
  colormap = x11().genId();

  auto colormap_cookie = xcb_create_colormap(
    connection, XCB_COLORMAP_ALLOC_NONE, colormap,
    screen->root, visual ? visual : screen->root_visual
  );
  auto err = xcb_request_check(connection, colormap_cookie);
  if(!err) return true;

  // There was an error - cleanup and signal failure
  free(err);
  return false;
}

X11Window::X11Window() :
  p(nullptr)
{
}

X11Window::~X11Window()
{
  delete p;
}

auto X11Window::create() -> IWindow&
{
  p = new pX11Window();

  if(!p->init(geometry_, background_)) throw X11InternalError();

  return *this;
}

auto X11Window::show() -> IWindow&
{
  assert(p && "can only be called after create()!");

  xcb_map_window(p->connection, p->window);
  x11().flush();    // Make sure the request gets fulfilled
                    //   before the method returns
  return *this;
}

auto X11Window::drawString(const std::string& str, const Geometry& geom, const Color& color,
    const std::string& font_name) -> IWindow&
{
  auto font = p->font(font_name.empty() ? "fixed" : font_name);
  if(!font) throw X11NoSuchFontError();

  auto gc = p->createGc(
    XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT,
    gc_args(color.bgr(), background_.bgr(), font.value())
  );
  if(!gc) throw X11InternalError();
  
  auto draw_cookie = xcb_image_text_8_checked(
      p->connection, str.size(), p->window, gc.value(), geom.x, geom.y, str.data()
  );
  auto err = xcb_request_check(p->connection, draw_cookie);
  if(err) {
    free(err);
    throw X11InternalError();
  }

  // Success => err == nulptr, so no need to cleanup
  xcb_free_gc(p->connection, gc.value());

  return *this;
}

auto X11Window::destroy() -> IWindow&
{
  delete p;
  p = nullptr;    // Make sure a double-free is not executed
                  //   during the destructor
  return *this;
}

auto X11Window::windowHandle() -> X11WindowHandle
{
  assert(p);

  return p->window;
}

auto X11Window::recreateWithVisualId(u8 depth, u32 visual_id) -> bool
{
  xcb_destroy_window(p->connection, p->window);
  xcb_free_colormap(p->connection, p->colormap);

  p->window = 0;
  p->colormap = 0;

  // When creating a window with an arbitrarily chosen visual
  //   we must also create a colormap with a visual which
  //   is the same as the one chosen for the window.
  //  - Failing to do so results in a BadMatch X server
  //    error when calling xcb_create_window()/XCreateWindow()
  if(!p->createColormap(visual_id)) return false;
  if(!p->createWindow(geometry_, background_, depth, visual_id)) return false;

  // Newly created windows are
  //   invisible by default
  show();

  return true;
}

}
