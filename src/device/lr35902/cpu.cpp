#include <device/lr35902/cpu.h>

#include <bus/memorymap.h>

namespace brgb::lr35902 {

auto Processor::connect(SystemBus *sys_bus) -> void
{
  bus_.reset(ProcessorBus::for_device(sys_bus, this));
}

auto Processor::bus() -> ProcessorBus&
{
  return *bus_;
}

}
