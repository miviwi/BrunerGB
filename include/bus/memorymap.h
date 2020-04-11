#pragma once

#include <types.h>
#include <bus/device.h>
#include <bus/mappedrange.h>

#include <type_traits>
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

  using AddressRange = std::pair<Address /* lo */, Address /* hi */>;

  enum : Address {
    AddressRangeInvalid = (Address)~0ull,
  };

  auto r(const char *address_range) -> BusReadHandler&;
  auto w(const char *address_range) -> BusWriteHandler&;

  // Lookup the designated BusReadHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupR(Address addr) -> BusReadHandler *;

  // Lookup the designated BusWriteHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupW(Address addr) -> BusWriteHandler *;

private:
  Address mask_;
  std::vector<AddressRange> ranges_;

  std::vector<MappedAddressRange> mapped_ranges_;
};

}
