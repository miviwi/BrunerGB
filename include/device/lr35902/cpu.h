#pragma once

#include <bus/bus.h>
#include <bus/device.h>

#include <device/lr35902/registers.h>

namespace brgb::lr35902 {

class Processor : public IBusDevice {
public:
  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;

protected:
  Bus<16> *bus_;

private:

  Registers r;
};

}
