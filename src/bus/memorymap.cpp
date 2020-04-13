#include <bus/memorymap.h>
#include <bus/mappedrange.h>

#include <algorithm>

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

        // Update cummulative range for the DeviceMemoryMap
        read_abs_.lo = std::min(read_abs_.lo, h->lo);
        read_abs_.hi = std::max(read_abs_.hi, h->hi);
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

        // Update cummulative range for the DeviceMemoryMap
        write_abs_.lo = std::min(write_abs_.lo, h->lo);
        write_abs_.hi = std::max(write_abs_.hi, h->hi);
    });

  // Pass the BusThransactionHandlerSetRef to the caller so it can be setup
  setup_handler(set);

  return *this;
}

auto DeviceMemoryMap::lookupR(Address addr) const -> BusReadHandler *
{
  if(!straddlesR(addr)) return nullptr;      // Early-out

  for(const auto& h : read_) {
    if(h->hi < addr || h->lo > addr) continue;

    return (BusReadHandler *)h.get();
  }

  // No handler registered for 'addr'
  return nullptr;
}

auto DeviceMemoryMap::lookupW(Address addr) const -> BusWriteHandler *
{
  if(!straddlesW(addr)) return nullptr;      // Early-out

  for(const auto& h : write_) {
    if(h->hi < addr || h->lo > addr) continue;

    return (BusWriteHandler *)h.get();
  }

  // No handler registered for 'addr'
  return nullptr;
}

auto DeviceMemoryMap::straddlesR(Address addr) const -> bool
{
  return addr >= read_abs_.lo && addr <= read_abs_.hi;
}

auto DeviceMemoryMap::straddlesW(Address addr) const -> bool
{
  return addr >= write_abs_.lo && addr <= write_abs_.hi;
}

}
