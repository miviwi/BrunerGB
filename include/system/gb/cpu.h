#pragma once

#include <device/sm83/cpu.h>

#include <bus/bus.h>
#include <bus/memorymap.h>

namespace brgb::gb {

class CPU final : public sm83::Processor {
public:
  virtual auto attach(SystemBus *sys_bus, IBusDevice *target) -> DeviceMemoryMap* final;
  virtual auto detach(DeviceMemoryMap *map) -> void final;

  virtual auto power() -> void final;
  virtual auto main() -> void final;

protected:
  virtual auto read(u16 addr) -> u8 final;
  virtual auto write(u16 addr, u8 data) -> void final;

private:

};

}
