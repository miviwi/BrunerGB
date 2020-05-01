#include <device/huc6280/cpu.h>

#include <bus/memorymap.h>

#include <cassert>

namespace brgb::huc6280 {

auto Processor::connect(SystemBus *sys_bus) -> void
{
  bus_.reset(ProcessorBus::for_device(sys_bus, this));
}

auto Processor::bus() -> ProcessorBus&
{
  return *bus_;
}

auto Processor::power() -> void
{
}

}
