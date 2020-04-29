#include "bus/bus.h"
#include <system/gb/gb.h>

#include <cassert>

namespace brgb {

Gameboy::Gameboy() :
  bus_(new SystemBus()),

  cpu_(new gb::CPU())
{
}

auto Gameboy::init() -> Gameboy&
{

  return *this;
}

auto Gameboy::power() -> void
{
  assert(was_init_ && "init() MUST be called before power()!");

  cpu().power();
}

auto Gameboy::cpu() -> gb::CPU&
{
  return *cpu_;
}

}
