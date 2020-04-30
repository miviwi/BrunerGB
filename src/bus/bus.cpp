#include <bus/bus.h>
#include <bus/device.h>
#include <bus/memorymap.h>

#include <utility>

#include <cassert>

namespace brgb {

auto SystemBus::addressSpaceFactory(IBusDevice *device) const -> IAddressSpace *
{
  assert(addrspace_factory_ &&
      "SystemBus::addressSpaceFactory() called without a valid AddressSpaceFactory!");

  auto dev_token = device->deviceToken();

  return addrspace_factory_(dev_token);
}

auto SystemBus::addressSpaceFactory(AddressSpaceFactory factory) -> SystemBus&
{
  addrspace_factory_ = std::move(factory);

  return *this;
}

auto SystemBus::createMap(IBusDevice *device) -> DeviceMemoryMap *
{
  IAddressSpace *addrspace = deviceAddressSpace(device);

  // Allocate a clean DeviceMemoryMap...
  auto device_memmap = new DeviceMemoryMap();

  //  ...and map it into device's address space
  auto device_memmap_ptr = addrspace->mapDevice(device_memmap);

  return device_memmap_ptr.get();
}

auto SystemBus::deviceAddressSpace(IBusDevice *device) -> IAddressSpace *
{
  auto it = devices_.find(device);
  if(it == devices_.end()) return createAddressSpace(device);

  // 'device' has been registered previously
  return it->second.get();
}

auto SystemBus::createAddressSpace(IBusDevice *device) -> IAddressSpace *
{
  // Allocate a clean AddressSpace...
  auto device_addrspace = std::shared_ptr<IAddressSpace>(addressSpaceFactory(device));

  //  ...and place it in the map
  auto [it, inserted] = devices_.emplace(device, device_addrspace);
  assert(inserted && "SystemBus::createAddressSpace() called twice!");

  return it->second.get();
}

template <size_t AddressWidth>
Bus<AddressWidth>::Bus(SystemBus *sys_bus, IBusDevice *device) :
  sys_bus_(sys_bus)
{
  addr_space_ = sys_bus->deviceAddressSpace<AddressWidth>(device);
}

template <size_t AddressWidth>
auto Bus<AddressWidth>::for_device(SystemBus *sys_bus, IBusDevice *device) -> Bus *
{
  return new Bus(sys_bus, device);
}

template <size_t AddressWidth>
auto Bus<AddressWidth>::readByte(Address addr) -> u8
{
  assert(addr_space_ &&
      "Bus<AddressWidth>::readByte() called without assiging an AddressSpace!");

  return addr_space_->readByte(addr);
}

template <size_t AddressWidth>
auto Bus<AddressWidth>::readWord(Address addr) -> u16
{
  assert(addr_space_ &&
      "Bus<AddressWidth>::readWord() called without assiging an AddressSpace!");

  return addr_space_->readWord(addr);
}

template <size_t AddressWidth>
auto Bus<AddressWidth>::writeByte(Address addr, u8 data) -> void
{
  assert(addr_space_ &&
      "Bus<AddressWidth>::writeByte() called without assiging an AddressSpace!");

  addr_space_->writeByte(addr, data);
}

template <size_t AddressWidth>
auto Bus<AddressWidth>::writeWord(Address addr, u16 data) -> void
{
  assert(addr_space_ &&
      "Bus<AddressWidth>::writeWord() called without assiging an AddressSpace!");

  addr_space_->writeWord(addr, data);
}

template class Bus<16>;

}
