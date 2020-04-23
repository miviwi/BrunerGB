#include <sched/device.h>

namespace brgb {

auto ISchedDevice::clock() -> Clock
{
  return clock_;
}

auto ISchedDevice::clock(Clock clk) -> ISchedDevice&
{
  clock_ = clk;

  return *this;
}

auto ISchedDevice::tick(Clock ticks) -> ISchedDevice&
{
  clock_ += ticks*scalar_;

  return *this;
}

auto ISchedDevice::frequency(double freq) -> ISchedDevice&
{
  frequency_ = freq+0.5;
  scalar_ = Second / frequency_;

  return *this;
}

auto ISchedDevice::scheduler() -> Scheduler *
{
  return sched_;
}

}
