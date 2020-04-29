#include <system/gb/cpu.h>

#include <bus/bus.h>
#include <bus/memorymap.h>
#include <device/sm83/cpu.h>

#include <cassert>

namespace brgb::gb {

auto CPU::attach(SystemBus *sys_bus, IBusDevice *target) -> DeviceMemoryMap*
{
  assert(0);
  return nullptr;
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
}

auto CPU::read(u16 addr) -> u8
{
  u8 data = bus_->readByte(addr);

  // 1 memory cycle = 4 internal cycles (t-cycles)
  tick(4);

  return data;
}

auto CPU::write(u16 addr, u8 data) -> void
{
  bus_->writeByte(addr, data);

  tick(4);
}

}
