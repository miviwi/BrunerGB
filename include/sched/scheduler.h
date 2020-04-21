#pragma once

#include <types.h>
#include <sched/thread.h>
#include <sched/device.h>

#include <vector>

// libco
#include <libco/libco.h>

namespace brgb {

class Scheduler {
public:
  enum Mode {
    ModeInvalid,

    // Run device threads until an arbitrary
    //   ISchedDevice::Event occurs
    Run,

    // Run device threads to a sync point, when
    //   execution can safely be paused (i.e.
    //   an ISchedDevice::Sync event)
    Sync,

    // Used internally by the Scheduler
    SyncPrimary, SyncAux,
  };

  using DeviceEvent = ISchedDevice::Event;

  auto threadById(Thread::Id id) -> Thread::Ptr;

  // Returns 'true' if the thread was successfully
  //   added to the Scheduler's list
  auto add(Thread::Ptr thread) -> bool;

  // Resets the clock's of all the device's and sets
  //   the primary thread to the one given
  auto power(Thread::Ptr primary) -> Scheduler&;

  // Execute device threads until a synchronization
  //   event (returned by the call) occurs
  auto run(Mode mode) -> DeviceEvent;

  // Return execution to the cothread which called run()
  //   (the host thread) from a device thread
  auto yield(DeviceEvent event) -> Scheduler&;

  // Mark a sync point from a device thread, where execution
  //   can be safely paused
  auto sync() -> Scheduler&;

  // Returns 'true' if execution is in the middle of
  //   a run(Scheduler::Sync) call (after the primary
  //   phase)
  //  - Used to avoid races between Threads, which
  //    belong to a single ISchedDevice
  auto duringSync() -> bool;

private:
  // If two threads have a clock of 0, it is ambiguous which one to
  //   select first, to resolve this an integer unique for each
  //   Thread is combined with it's clock and this value
  //   is used to select the next thread to run
  auto uniqueId() -> Thread::Id;

  auto hasThread(Thread *ptr) -> bool;

  // Get the clock of the device furthest ahead
  auto aheadClock() -> ISchedDevice::Clock;
  // Get the clock of the device furthest behind
  auto behindClock() -> ISchedDevice::Clock;

  Mode mode_ = ModeInvalid;

  // Event passed to Scheduler::yield(), stored here
  //   so it can be returned back to the host thread
  DeviceEvent yield_event_ = ISchedDevice::None;

  // Thread used to exit the Scheduler (the main one)
  cothread_t host_ = nullptr;

  // Thread used to re-enter the Scheduler
  cothread_t resume_ = nullptr;

  std::vector<Thread::Ptr> threads_;

  // Thread to which all others are synchronized (i.e.
  //   the one which is always scheduled first when
  //   synchronization is requested)
  Thread::Ptr primary_;



};

}
