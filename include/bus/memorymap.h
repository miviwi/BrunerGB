#pragma once

#include <types.h>
#include <bus/device.h>
#include <bus/mappedrange.h>

#include <type_traits>
#include <memory>
#include <vector>
#include <utility>

namespace brgb {

// Forward declarations
class DeviceMemoryMap;

class IAddressSpace {
public:
  virtual auto mapDevice(DeviceMemoryMap *device_memmap) -> void = 0;
};

template <size_t AddressWidth>
class AddressSpace : public IAddressSpace {
public:
  using Address = std::conditional<
    AddressWidth >= 32, u32, std::conditional<
    AddressWidth >= 16, u16,
 /* AddressWidth >= 8 */ u8
   >>;

  virtual auto mapDevice(DeviceMemoryMap *device_memmap) -> void final;

  auto readByte(Address addr) -> u8;
  auto readWord(Address addr) -> u16;

  auto writeByte(Address addr, u8 data) -> void;
  auto writeWord(Address addr, u16 data) -> void;

private:
};

class DeviceMemoryMap {
public:
  using Address = u64;     // TODO: Somehow fudge this type into
                           //   a template parameter (?)

  // Returns a new BusReadHandler/BusWriteHandler for r()/w()
  //  respectively whose 
  //    - lo/hi Addresses
  //    - mask
  //    - address_range (debug variable)
  //  are set appropriately, but the rest of the members have
  //  their default value
  //     - 'address_range' is specified in a format which allows
  //       multiple disjoint ranges to be used for a given handler
  //       ex.
  //                 0x1000-0x1fff
  //                 0x1000-0x1fff,0x3000-0x3fff

  auto r(const char *address_range, u64 mask = ~0ull) -> BusTransactionHandlerSetRef;
  auto w(const char *address_range, u64 mask = ~0ull) -> BusTransactionHandlerSetRef;

  // Lookup the designated BusReadHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupR(Address addr) const -> BusReadHandler *;

  // Lookup the designated BusWriteHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupW(Address addr) const -> BusWriteHandler *;

private:
  std::vector<BusTransactionHandler::Ptr> read_, write_;
};

}
