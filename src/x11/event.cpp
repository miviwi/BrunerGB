#include <x11/event.h>
#include <x11/connection.h>
#include <x11/window.h>

// X11/xcb headers
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

#include <cassert>
#include <cstdlib>

#include <utility>

namespace brdrive {

template <typename T = xcb_generic_event_t>
static inline auto xcb_ev(X11EventHandle ev) -> const T*
{
  return (T *)ev;
}

X11Event::X11Event(X11EventHandle ev) :
  Event(type_from_handle(ev)),
  event_(ev)
{
}

X11Event::~X11Event()
{
  free(event_);
}

auto X11Event::from_X11EventHandle(X11EventLoop *event_loop, X11EventHandle ev) -> Event::Ptr
{
  // Check if 'ev' is a valid handle
  if(!ev) return Event::Ptr();

  auto event = Event::Ptr();
  auto event_type = type_from_handle(ev);

  // Event::Invalid signals that an event which should be squashed
  //   (e.g. XCB_EXPOSE) has occured, so return a nullptr in that case
  if(event_type == Event::Invalid) return Event::Ptr();

  switch(event_type) {
  case Event::KeyUp:
  case Event::KeyDown:
    event.reset(new X11KeyEvent(event_loop, ev));
    break;
    
  case Event::MouseMove:
  case Event::MouseDown:
  case Event::MouseUp:
    event.reset(new X11MouseEvent(event_loop, ev));
    break;
  }

  return std::move(event);
}

auto X11Event::x11_response_type(X11EventHandle ev) -> X11ResponseType
{
  assert(ev);

  return XCB_EVENT_RESPONSE_TYPE(xcb_ev(ev));
}

auto X11Event::type_from_handle(X11EventHandle ev) -> Event::Type
{
  assert(ev);

  auto response_type = x11_response_type(ev);
  switch(response_type) {
  case XCB_KEY_PRESS:   return Event::KeyDown;
  case XCB_KEY_RELEASE: return Event::KeyUp;

  case XCB_MOTION_NOTIFY:  return Event::MouseMove;
  case XCB_BUTTON_PRESS:   return Event::MouseDown;
  case XCB_BUTTON_RELEASE: return Event::MouseUp;
  }

  return Event::Invalid;
}

auto X11Event::is_internal(X11EventHandle ev) -> bool
{
  auto type = x11_response_type(ev);
  switch(type) {
  case XCB_EXPOSE: return true;
  }

  return false;
}

X11KeyEvent::X11KeyEvent(X11EventLoop *event_loop, X11EventHandle ev) :
  X11Event(ev),
  keycode_(Key::Invalid), keysym_(Key::Invalid)
{
  switch(type()) {
  case Event::KeyDown: {
    auto key_press = xcb_ev<xcb_key_press_event_t>(ev);

    keycode_ = key_press->detail;
    keysym_  = x11().keycodeToKeysym(key_press->detail);
    break;
  }

  case Event::KeyUp: {
    auto key_release = xcb_ev<xcb_key_release_event_t>(ev);

    keycode_ = key_release->detail;
    keysym_  = x11().keycodeToKeysym(key_release->detail);
    break;
  }

  default: assert(0);    // Unreachable
  }
}

X11KeyEvent::~X11KeyEvent()
{
}

auto X11KeyEvent::code() const -> u32
{
  return keycode_;
}

auto X11KeyEvent::sym() const -> u32
{
  return keysym_;
}

X11MouseEvent::X11MouseEvent(X11EventLoop *event_loop, X11EventHandle ev) :
  X11Event(ev),
  point_(Vec2<i16>::zero()), delta_(Vec2<i16>::zero())
{
  switch(type()) {
  case Event::MouseMove: {
    auto mouse_move = xcb_ev<xcb_motion_notify_event_t>(ev);

    auto mouse_last = event_loop->mouse_last_;

    auto point = Vec2<i16>{ mouse_move->event_x, mouse_move->event_y };
    auto last  = (mouse_last.x != IMouseEvent::InvalidCoord) ? mouse_last : Vec2<i16>::zero();

    point_ = point;
    delta_ = { (i16)(point.x - last.x), (i16)(point.y - last.y) };
    break;
  }

  case Event::MouseDown: {
    auto mouse_down = xcb_ev<xcb_button_press_event_t>(ev);

    point_ = { mouse_down->event_x, mouse_down->event_y };
    break;
  }

  case Event::MouseUp: {
    auto mouse_up = xcb_ev<xcb_button_release_event_t>(ev);

    point_ = { mouse_up->event_x, mouse_up->event_y };
    break;
  }

  default: assert(0);     // Unreachable
  }
}

X11MouseEvent::~X11MouseEvent()
{
}

auto X11MouseEvent::point() const -> Vec2<i16>
{
  return point_;
}

auto X11MouseEvent::delta() const -> Vec2<i16>
{
  return delta_;
}

X11EventLoop::X11EventLoop() :
  mouse_last_({ InvalidCoord }), mouse_buttons_last_(0),
  key_modifiers_(0)
{
}

X11EventLoop::~X11EventLoop()
{
}

auto X11EventLoop::initInternal() -> bool
{
#if 0
  auto connection = x11().connection<xcb_connection_t>();
  auto window = (X11Window *)IEventLoop::window();

  auto pointer_query_cookie = xcb_query_pointer(
      connection, window->windowHandle()
  );

  xcb_generic_error_t *err = nullptr;
  auto pointer_reply = xcb_query_pointer_reply(
      connection, pointer_query_cookie, &err
  );
  if(err) {      // If there was an error - cleanup and inform the parent class
    free(err);
    free(pointer_reply);

    return false;
  }

  mouse_last_ = { pointer_reply->win_x, pointer_reply->win_y };
  free(pointer_reply);
#endif

  return true;
}

auto X11EventLoop::pollEvent() -> Event::Ptr
{
  auto connection = x11().connection<xcb_connection_t>();

  // Poll until a non-internal event is returned
  xcb_generic_event_t *ev = nullptr;
  while( (ev = xcb_poll_for_event(connection)) ) {
    if(!X11Event::is_internal(ev)) break;

    // Process any internal events so they don't
    //   escape from the X11EventLoop
    processInternal(ev);
  }

  if(!ev) return Event::Ptr();

  // We've obtained a valid non-internal event
  auto event = X11Event::from_X11EventHandle(this, ev);
  assert(event.get());

  // Make sure the internal data structures are updated
  updateInternalData(event.get());

  return std::move(event);
}

auto X11EventLoop::waitEvent() -> Event::Ptr
{
  auto connection = x11().connection<xcb_connection_t>();
  auto event = Event::Ptr();

  // Spin until a non-internal event is returned
  xcb_generic_event_t *ev = nullptr;
  while(true) {
    ev = xcb_wait_for_event(connection);
    if(!ev) return QuitEvent::alloc();     // NULL event signifies the window was closed -
                                           //   so signal the application should exit
    if(!X11Event::is_internal(ev)) {
      event = X11Event::from_X11EventHandle(this, ev);
      if(event) break;
    }

    // Process any internal events so they don't
    //   escape from the X11EventLoop
    processInternal(ev);
  }

  assert(event.get());

  // Make sure the internal data structures are updated
  updateInternalData(event.get());

  return std::move(event);
}

auto X11EventLoop::queueEmptyInternal() const -> bool
{
  return xcb_poll_for_queued_event(x11().connection<xcb_connection_t>());
}

void X11EventLoop::processInternal(X11EventHandle ev)
{
  auto response_type = X11Event::x11_response_type(ev);
  switch(response_type) {
  case XCB_EXPOSE: break;
  }
}

void X11EventLoop::updateInternalData(const Event *event)
{
  switch(event->type()) {
  case Event::MouseMove:       // Update the last mouse position when
  case Event::MouseDown:       //   a mouse event occurs
  case Event::MouseUp: {
    auto mouse = (const IMouseEvent *)event;
    mouse_last_ = mouse->point();

    break;
  }
  }
}

}
