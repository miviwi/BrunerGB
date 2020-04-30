#pragma once

#include <types.h>

namespace brgb {

// Forward declarations
class SystemBus;
class DeviceMemoryMap;

class IBusDevice {
public:
  using DeviceToken = u32;

  virtual auto deviceToken() -> DeviceToken = 0;

  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;
};

}
