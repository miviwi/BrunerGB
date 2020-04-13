#include <bus/memorymap.h>

namespace brgb {

// TODO: stub!
template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::mapDevice(
    DeviceMemoryMap *device_memmap
  ) -> DeviceMemoryMap::Ptr
{
  return devices_.emplace_back(device_memmap);
}

// TODO: stub!
template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::readByte(Address addr) -> u8
{
  return 0;
}

// TODO: stub!
template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::readWord(Address addr) -> u16
{
  return 0;
}

// TODO: stub!
template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::writeByte(Address addr, u8 data) -> void
{
}

// TODO: stub!
template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::writeWord(Address addr, u16 data) -> void
{
}

template class AddressSpace<16>;

auto DeviceMemoryMap::r(
    const char *address_range, SetupHandlerFn setup_handler
  ) -> DeviceMemoryMap&
{
  auto set = BusReadHandlerSet::from_address_range(address_range);

  // Store all the BusTransactionHandlers encompassed by the 'address_range'
  set.get()
    .eachPtr([this](BusTransactionHandler::Ptr h) {
        read_.emplace_back(h);
    });

  // Pass the BusThransactionHandlerSetRef to the caller so it can be setup
  setup_handler(set);

  return *this;
}

auto DeviceMemoryMap::w(
    const char *address_range, SetupHandlerFn setup_handler
  ) -> DeviceMemoryMap&
{
  auto set = BusWriteHandlerSet::from_address_range(address_range);

  // Store all the BusTransactionHandlers encompassed by the 'address_range'
  set.get()
    .eachPtr([this](BusTransactionHandler::Ptr h) {
        write_.emplace_back(h);
    });

  // Pass the BusThransactionHandlerSetRef to the caller so it can be setup
  setup_handler(set);

  return *this;
}

}
