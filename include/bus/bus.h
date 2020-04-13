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

  auto deviceAddressSpace(IBusDevice *device) -> IAddressSpace *;

  template <size_t AddressWidth>
  auto deviceAddressSpace(IBusDevice *device) -> AddressSpace<AddressWidth> *
  {
    return (AddressSpace<AddressWidth> *)deviceAddressSpace(device);
  }

private:
  // Can be called ONLY when devices_.find(device) == devices_.end()
  //   i.e. when a given device HASN'T been previously registered
  //   with the SystemBus
  auto createAddressSpace(IBusDevice *device) -> IAddressSpace *;

  AddressSpaceFactory addrspace_factory_;

  std::unordered_map<IBusDevice *, std::shared_ptr<IAddressSpace>> devices_;
};

template <size_t AddressWidth>
class Bus {
public:
  using Address = typename AddressSpace<AddressWidth>::Address;

  static auto for_device(SystemBus *sys_bus, IBusDevice *device) -> Bus *;

  auto readByte(Address addr) -> u8;
  auto readWord(Address addr) -> u16;

  auto writeByte(Address addr, u8 data) -> void;
  auto writeWord(Address addr, u16 data) -> void;

private:
  Bus(SystemBus *sys_bus, IBusDevice *device);

  SystemBus *sys_bus_ = nullptr;

  AddressSpace<AddressWidth> *addr_space_ = nullptr;
};

}
