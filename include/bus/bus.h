#pragma once

#include <types.h>

namespace brgb {

// Forward declarations
template <size_t AddressWidth>
class AddressSpace;

class DeviceMemoryMap;

class SystemBus {
public:
  auto createMap() -> DeviceMemoryMap;

private:
};

template <size_t AddressWidth>
class Bus {
public:

private:
  SystemBus *sys_bus_;

  AddressSpace<AddressWidth> *addr_space_;
};

}
