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

template class Bus<16>;

}
