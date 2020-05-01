#pragma once

#include <util/primitive.h>
#include <bus/bus.h>
#include <bus/device.h>
#include <sched/device.h>

#include <memory>

namespace brgb::huc6280 {

class Processor : public IBusDevice, public ISchedDevice {
public:
  using ProcessorBus = Bus<21>;

  auto connect(SystemBus *sys_bus) -> void;

  auto bus() -> ProcessorBus&;

  virtual auto deviceToken() -> DeviceToken = 0;

  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap* = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;

  virtual auto power() -> void;
  virtual auto main() -> void = 0;

protected:
  virtual auto read(u21 addr) -> u8 = 0;
  virtual auto write(u21 addr, u8 data) -> void = 0;

  auto instruction() -> void;

  std::unique_ptr<ProcessorBus> bus_;

private:
  auto opcode() -> u8;

  auto operand8() -> u8;
  auto operand16() -> u16;
};

}
