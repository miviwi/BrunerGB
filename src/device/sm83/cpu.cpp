#include <device/sm83/cpu.h>

#include <bus/memorymap.h>

namespace brgb::sm83 {

auto Processor::connect(SystemBus *sys_bus) -> void
{
  bus_.reset(ProcessorBus::for_device(sys_bus, this));
}

auto Processor::bus() -> ProcessorBus&
{
  return *bus_;
}

}
