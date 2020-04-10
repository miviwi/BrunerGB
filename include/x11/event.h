#pragma once

#include <x11/x11.h>
#include <window/event.h>
#include <window/geometry.h>

#include <memory>

namespace brdrive {

// Forward declaration
class X11EventLoop;

using X11ResponseType = u8;
using X11EventHandle  = void /* xcb_generic_event_t */ *;

class X11Event : public Event {
public:
  X11Event(const X11Event&) = delete;
  virtual ~X11Event();

  static auto from_X11EventHandle(X11EventLoop *event_loop, X11EventHandle ev) -> Event::Ptr;

protected:
  friend X11EventLoop;

  X11Event(X11EventHandle ev);

  static auto x11_response_type(X11EventHandle ev) -> X11ResponseType;
  static auto type_from_handle(X11EventHandle ev) -> Event::Type;
  static auto is_internal(X11EventHandle ev) -> bool;

  X11EventHandle event_;
};

class X11KeyEvent : public X11Event, public IKeyEvent {
public:
  virtual ~X11KeyEvent();

  virtual auto code() const -> u32;
  virtual auto sym() const -> u32;

private:
  friend X11Event;

  X11KeyEvent(X11EventLoop *event_loop, X11EventHandle ev);

  u32 keycode_;
  u32 keysym_;
};

class X11MouseEvent : public X11Event, public IMouseEvent {
public:
  virtual ~X11MouseEvent();

  virtual auto point() const -> Vec2<i16>;
  virtual auto delta() const -> Vec2<i16>;

private:
  friend X11Event;

  X11MouseEvent(X11EventLoop *event_loop, X11EventHandle ev);

  Vec2<i16> point_;
  Vec2<i16> delta_;
};

class X11EventLoop : public IEventLoop {
public:
  X11EventLoop();
  virtual ~X11EventLoop();

protected:
  virtual auto initInternal() -> bool;
  virtual auto queueEmptyInternal() const -> bool;

  virtual auto pollEvent() -> Event::Ptr;
  virtual auto waitEvent() -> Event::Ptr;

private:
  friend X11KeyEvent;
  friend X11MouseEvent;

  enum : i16 {
    InvalidCoord = ~0,
  };

  void processInternal(X11EventHandle ev);
  void updateInternalData(const Event *event);

  Vec2<i16> mouse_last_;
  u16 mouse_buttons_last_;
  u32 key_modifiers_;
};

}
