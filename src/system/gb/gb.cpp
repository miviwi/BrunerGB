#include <system/gb/gb.h>

#include <sched/thread.h>

#include <cassert>

namespace brgb {

Gameboy::Gameboy() :
  bus_(new SystemBus()),

  cpu_(new gb::CPU())
{
}

auto Gameboy::init() -> Gameboy&
{
  sched.add(Thread::create(SystemClock, cpu_.get()));
  // TODO: create threads for the rest of the devices

  was_init_ = true;

  return *this;
}

auto Gameboy::power() -> void
{
  assert(was_init_ && "init() MUST be called before power()!");

  cpu().power();
  // TODO: power up the rest of the devices

  // Make the CPU the primary device
  sched.power(cpu_.get());
}

auto Gameboy::cpu() -> gb::CPU&
{
  return *cpu_;
}

}
