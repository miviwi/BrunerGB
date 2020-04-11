#pragma once

#include <types.h>
#include <bus/device.h>

#include <type_traits>

namespace brgb {

// Forward declarations
class BusReadHandler;
class BusWriteHandler;

template <size_t AddressWidth>
class AddressSpace {
public:
  using Address = std::conditional<
    AddressWidth >= 32, u32, std::conditional<
    AddressWidth >= 16, u16,
 /* AddressWidth >= 8 */ u8
   >>;

  auto readByte(Address addr) -> u8;
  auto readWord(Address addr) -> u16;

  auto writeByte(Address addr, u8 data) -> void;
  auto writeWord(Address addr, u16 data) -> void;

private:
};

class DeviceMemoryMap {
public:
  auto r(const char *address_range) -> BusReadHandler&;
  auto w(const char *address_range) -> BusWriteHandler&;

private:
};

}
