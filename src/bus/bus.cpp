#include <bus/bus.h>
#include <bus/device.h>
#include <bus/memorymap.h>

#include <utility>

#include <cassert>

namespace brgb {

auto SystemBus::addressSpaceFactory() const -> IAddressSpace *
{
  assert(addrspace_factory_ &&
      "SystemBus::addressSpaceFactory() called without a valid AddressSpaceFactory!");

  return addrspace_factory_();
}

auto SystemBus::addressSpaceFactory(AddressSpaceFactory factory) -> SystemBus&
{
  addrspace_factory_ = std::move(factory);

  return *this;
}

auto SystemBus::createMap(IBusDevice *device) -> DeviceMemoryMap *
{
  auto it = devices_.find(device);
  if(it == devices_.end()) {     // First time createMap() was called for this device
    // Allocate a new clean AddressSpace...
    auto device_addrspace = std::shared_ptr<IAddressSpace>(addressSpaceFactory());

    //  ...and place it in the map
    auto [inserted_it, inserted] = devices_.insert(
        { device, device_addrspace }
    );

    assert(inserted);

    it = inserted_it;
  }

  assert(it != devices_.end());

  IAddressSpace& addrspace = *it->second;

  // Allocate a clean DeviceMemoryMap...
  auto device_memmap = new DeviceMemoryMap();

  //  ...and map it into device's address space
  auto device_memmap_ptr = addrspace.mapDevice(device_memmap);

  return device_memmap_ptr.get();
}

auto SystemBus::deviceAddressSpace(IBusDevice *device) -> IAddressSpace *
{
  auto it = devices_.find(device);
  if(it == devices_.end()) {                  // 'device' hasn't been registered
        // Allocate a new clean AddressSpace...
    auto device_addrspace = std::shared_ptr<IAddressSpace>(addressSpaceFactory());

    //  ...and place it in the map
    auto [inserted_it, inserted] = devices_.insert(
        { device, device_addrspace }
    );

    assert(inserted);

    it = inserted_it;
  }

  return it->second.get();
}

template <size_t AddressWidth>
Bus<AddressWidth>::Bus(SystemBus *sys_bus, IBusDevice *device) :
  sys_bus_(sys_bus)
{
  addr_space_ = (AddressSpace<AddressWidth> *)sys_bus->deviceAddressSpace(device);
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
