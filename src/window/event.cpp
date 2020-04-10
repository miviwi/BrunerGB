#include <window/event.h>
#include <window/window.h>

#include <cassert>

#include <deque>
#include <utility>

namespace brdrive {

IEventLoop::IEventLoop() :
  was_init_(false)
{
}

IEventLoop::~IEventLoop()
{
}

auto IEventLoop::init(IWindow *window) -> IEventLoop&
{
  window_ = window;

  if(!initInternal()) throw InitError();
  was_init_ = true;

  return *this;
}

auto IEventLoop::event(u32 flags) -> Event::Ptr
{
  assert(was_init_ && "init() wasn't called before using other methods!");

  auto event = Event::Ptr();
  if(!queue_.empty()) {    // First check if there are items in the queue...
    event = std::move(queue_.front());
    queue_.pop_front();

    return std::move(event);
  }

  // ...if there aren't any we must check if we should block
  if(flags & Block) {
    // ...we are in blocking mode so wait for an event...
    event = waitEvent();

    // ...and fill up the queue as much as possible to
    //    reduce overhead on future event() calls
    fillQueue();
  } else {
    // Try to fill up the queue...
    fillQueue();

    // ...and return the first event if we managed to get any
    if(!queue_.empty()) {
      event = std::move(queue_.front());
      queue_.pop_front();
    }
  }

  // If we are in non-blocking mode and events were neither queued
  //   nor have happened since last time the function was called
  //   nullptr will be returned here
  return std::move(event);
}

auto IEventLoop::queueEmpty() const -> bool
{
  if(queue_.empty()) return queueEmptyInternal();

  return false;   // !queue_.empty()
}

void IEventLoop::fillQueue()
{
  // Fill up the queue (as much as possible)...
  while(auto new_event = pollEvent()) queue_.push_back(std::move(new_event));
}

auto IEventLoop::window() -> IWindow*
{
  return window_;
}

Event::~Event()
{
}

auto Event::type() const -> Type
{
  return type_;
}

QuitEvent::QuitEvent() :
  Event(Quit)
{
}

IKeyEvent::~IKeyEvent()
{
}

IMouseEvent::~IMouseEvent()
{
}

}
