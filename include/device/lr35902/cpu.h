#pragma once

#include <bus/bus.h>
#include <bus/device.h>

#include <device/lr35902/registers.h>

#include <memory>

namespace brgb::lr35902 {

class Processor : public IBusDevice {
public:
  using ProcessorBus = Bus<16>;

  auto connect(SystemBus *sys_bus) -> void;

  auto bus() -> ProcessorBus&;

  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;

protected:
  std::unique_ptr<ProcessorBus> bus_;

private:

  Registers r;
};

}
