#pragma once

#include <types.h>

#include <memory>
#include <vector>
#include <unordered_map>

namespace brgb {

// Forward declarations
class IBusDevice;
class IAddressSpace;
class DeviceMemoryMap;

template <size_t AddressWidth>
class AddressSpace;

class SystemBus {
public:
  auto createMap(IBusDevice *device) -> DeviceMemoryMap;

private:
  std::unordered_map<IBusDevice *, std::shared_ptr<IAddressSpace>> devices_;
};

template <size_t AddressWidth>
class Bus {
public:
  using Address = typename AddressSpace<AddressWidth>::Address;

  auto readByte(Address addr) -> u8;
  auto readWord(Address addr) -> u16;

  auto writeByte(Address addr, u8 data) -> void;
  auto writeWord(Address addr, u16 data) -> void;

private:
  SystemBus *sys_bus_;

  AddressSpace<AddressWidth> *addr_space_;
};

}
