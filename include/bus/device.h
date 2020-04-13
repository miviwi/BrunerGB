#pragma once

#include <brgb.h>

namespace brgb {

// Forward declarations
class SystemBus;
class DeviceMemoryMap;

class IBusDevice {
public:
  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;
};

}
