#pragma once

#include <types.h>

#include <memory>
#include <functional>
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
  using AddressSpaceFactory = std::function<IAddressSpace *()>;

  // Returns an AddressSpace created by the cuurently used AddressSpaceFactory
  auto addressSpaceFactory() const -> IAddressSpace *;
   
  // Set the AddressSpaceFactory used by this SystemBus, which
  //   is supposed to return an AddressSpace<AddressWidth> of the
  //   desired 'AddressWidth' via an IAddressSpace*
  auto addressSpaceFactory(AddressSpaceFactory factory) -> SystemBus&;

  // Do NOT store the returned DeviceMemoryMap * to avoid memory leaks
  //   TODO: Make DeviceMemoryMap an internally ref-counted object (?)
  auto createMap(IBusDevice *device) -> DeviceMemoryMap *;

private:
  AddressSpaceFactory addrspace_factory_;

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
