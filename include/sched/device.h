#pragma once

#include "bus/device.h"
#include <types.h>

namespace brgb {

// Forward declarations
class Scheduler;
class Thread;

class ISchedDevice {
public:
  using Clock = u64;

  enum : Clock {
    Second = (Clock)-1 >> 1,
  };

  enum Event {
    None,
    Power,
    Tick, VideoFrame,
    Sync,
  };

  // Returns the number of clock ticks elapsed since
  //  the device's power-on
  auto clock() -> Clock;

  auto clock(Clock clk) -> ISchedDevice&;

  // Increments the clock()
  auto tick(Clock ticks = 1) -> ISchedDevice&;

  // Sets the device's clock frequency
  auto frequency(double freq) -> ISchedDevice&;

  // Called ONCE, when the emulator is run
  virtual auto power() -> void = 0;

  // Called CONTINUALLY by the Scheduler
  virtual auto main() -> void = 0;

protected:
  auto scheduler() -> Scheduler *;

private:
  friend Scheduler;
  friend Thread;

  // Device's clock frequency
  double frequency_ = 0.0;

  // Multiplication factor for 'ticks' passed to tick(),
  //   used like so:
  //       clock = ticks * scalar_
  //  - It's value is derived from the desired clock
  //    frequency for the device, it will be very big
  //    so care must be taken by the Scheduler to prevent
  //    overflowing 'clock_' - see Scheduler::yield()
  Clock scalar_ = 0;

  Clock clock_ = 0;

  // Scheduler which owns this ISchedDevice
  Scheduler *sched_ = nullptr;
};

}
