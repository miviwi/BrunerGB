#include <system/gb/cpu.h>

#include <bus/bus.h>
#include <bus/memorymap.h>
#include <device/sm83/cpu.h>

#include <cassert>

namespace brgb::gb {

auto CPU::deviceToken() -> DeviceToken
{
  return GameboyCPUDeviceToken;
}

auto CPU::attach(SystemBus *sys_bus, IBusDevice *target) -> DeviceMemoryMap*
{
  return sys_bus->createMap(this);
}

auto CPU::detach(DeviceMemoryMap *map) -> void
{
}

auto CPU::power() -> void
{
  sm83::Processor::power();
}

auto CPU::main() -> void
{
  // TODO: handle interrupts
  
  instruction();   // Fetch, decode and execute an instruction
}

auto CPU::read(u16 addr) -> u8
{
  u8 data = bus().readByte(addr);

  // 1 memory cycle = 4 internal cycles (t-cycles)
  tick(4);

  return data;
}

auto CPU::write(u16 addr, u8 data) -> void
{
  bus().writeByte(addr, data);

  tick(4);
}

}
