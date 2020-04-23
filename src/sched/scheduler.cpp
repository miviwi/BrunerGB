#include <sched/scheduler.h>

#include <algorithm>
#include <limits>

#include <cassert>

namespace brgb {

auto Scheduler::threadById(Thread::Id id) -> Thread::Ptr
{
  auto thread = Thread::Ptr();
  for(const auto& t : threads_) {
    if(t->id_ != id) continue;

    thread = t;
    break;
  }

  return thread;
}

auto Scheduler::add(Thread::Ptr thread) -> bool
{
  assert(thread->handle() && "attempted to add() an invalid Thread!");

  // Make sure 'thread' hasn't been added before
  if(hasThread(thread.get())) return false;

  thread->id_ = uniqueId();

  // Add the thread to the back of the queue
  thread->device()->clock_ = aheadClock() + thread->id_;

  thread->device()->sched_ = this;
  threads_.push_back(thread);

  return true;
}

auto Scheduler::power(ISchedDevice *primary) -> Scheduler&
{
  auto it_primary_thread = std::find_if(threads_.begin(), threads_.end(), [=](const auto& t) {
    return t->device() == primary;
  });

  assert(it_primary_thread != threads_.end() &&
      "Scheduler::power(): 'primary' not owned by this Scheduler!");

  auto& primary_thread = *it_primary_thread;

  // Set the primary thread...
  primary_ = primary_thread;
  resume_ = primary_thread->handle();

  //  ...and reset all the clocks
  for(auto& t : threads_) {
    t->device()->clock(t->id_);
  }

  return *this;
}

auto Scheduler::run(Mode mode) -> DeviceEvent
{
  if(mode == Run) {
    mode_ = mode;
    host_ = co_active();

    co_switch(resume_);

    return yield_event_;
  } else if(mode == Sync) {
    // Run the device thread until a sync point, when
    //   execution can safely be paused
    auto until_sync_point = [this](const Thread::Ptr& thread) {
      host_ = co_active();
      resume_ = thread->handle();

      do {
        co_switch(resume_);
      } while(yield_event_ != ISchedDevice::Sync);
    };

    // Run the primary thread first...
    mode_ = SyncPrimary;
    until_sync_point(primary_);

    for(const auto& t : threads_) {
      if(t == primary_) continue;

      //  ...and the rest of the threads
      mode_ = SyncAux;
      until_sync_point(t);
    }

    return ISchedDevice::Sync;
  }

  return ISchedDevice::None;
}

auto Scheduler::yield(DeviceEvent event) -> Scheduler&
{
  // Subtract the lowest clock value (i.e. the clock of the
  //   device furthest behind) from all the clocks
  //   to prevent overflow
  auto minimum = behindClock();
  for(auto& t : threads_) {
    t->device()->clock_ -= minimum;
  }

  yield_event_ = event;
  resume_ = co_active();     // Return execution to the currently
                             //   running device thread upon the
                             //   next run()
  co_switch(host_);
  
  return *this;
}

auto Scheduler::sync() -> Scheduler&
{
  // Yield to the host thread if we're in the middle
  //   of synchronization
  if(co_active() == primary_->handle()) {
    if(mode_ == SyncPrimary) yield(ISchedDevice::Sync);
  } else {
    if(mode_ == SyncAux) yield(ISchedDevice::Sync);
  }

  return *this;
}

auto Scheduler::duringSync() -> bool
{
  return mode_ == SyncAux;
}

auto Scheduler::syncWithAll() -> void
{
  // This will eventually attempt to sync the currently running
  //   device with itself, but this is safe as a device is never
  //   considered to be behind itself
  for(auto& t : threads_) {
    syncWith(t->device());
  }
}

auto Scheduler::syncWith(ISchedDevice *device) -> void
{
  auto it_self = std::find_if(threads_.begin(), threads_.end(), [](const Thread::Ptr& t) {
    return co_active() == t->thread_;
  });
  assert(it_self != threads_.end() && "current Thread not owned by this Scheduler!");

  auto& self = *it_self;
  while(device->clock() < self->device()->clock()) {
    if(duringSync()) break;

    co_switch(self->handle());
  }
}

auto Scheduler::uniqueId() -> Thread::Id
{
  Thread::Id id = 0;

  // Find the first unused id
  while(threadById(id)) id++;

  return id;
}

auto Scheduler::hasThread(Thread *ptr) -> bool
{
  for(const auto& t : threads_) {
    if(t.get() == ptr) return true;
  }

  return false;
}

auto Scheduler::aheadClock() -> ISchedDevice::Clock
{
  auto ahead = std::numeric_limits<ISchedDevice::Clock>::min();
  for(const auto& t : threads_) {
    ahead = std::max(ahead, t->device()->clock() - t->id_);
  }

  return ahead;
}

auto Scheduler::behindClock() -> ISchedDevice::Clock
{
  auto behind = std::numeric_limits<ISchedDevice::Clock>::max();
  for(const auto& t : threads_) {
    behind = std::min(behind, t->device()->clock() - t->id_);
  }

  return behind;
}

}
