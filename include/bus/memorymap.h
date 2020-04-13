#pragma once

#include <types.h>
#include <bus/device.h>
#include <bus/mappedrange.h>

#include <type_traits>
#include <memory>
#include <vector>
#include <functional>
#include <utility>

namespace brgb {

// Forward declarations
class IAddressSpace;

class DeviceMemoryMap {
public:
  using Address = u64;     // TODO: Somehow fudge this type into
                           //         a template parameter (?)

  using Ptr = std::shared_ptr<DeviceMemoryMap>;

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

  using SetupHandlerFn = std::function<void(BusTransactionHandlerSetRef&)>;

  auto r(
      const char *address_range, SetupHandlerFn setup_handler
    ) -> DeviceMemoryMap&;

  auto w(
      const char *address_range, SetupHandlerFn setup_handler
    ) -> DeviceMemoryMap&;

  // Lookup the designated BusReadHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupR(Address addr) const -> BusReadHandler *;

  // Lookup the designated BusWriteHandler for 'addr'
  //   - Can return NULL when there is no handler defined
  auto lookupW(Address addr) const -> BusWriteHandler *;

private:
  auto straddlesR(Address addr) const -> bool;
  auto straddlesW(Address addr) const -> bool;

  struct {
    Address lo = ~0ull;
    Address hi = 0;
  } read_abs_, write_abs_;

  std::vector<BusTransactionHandler::Ptr> read_, write_;
};

class IAddressSpace {
public:
  virtual auto mapDevice(DeviceMemoryMap *device_memmap) -> DeviceMemoryMap::Ptr = 0;
};

template <size_t AddressWidth>
class AddressSpace : public IAddressSpace {
public:
  using Address = std::conditional_t<
    AddressWidth >= 32, u32, std::conditional_t<
    AddressWidth >= 16, u16,
 /* AddressWidth >= 8 */ u8
   >>;

  virtual auto mapDevice(DeviceMemoryMap *device_memmap) -> DeviceMemoryMap::Ptr final;

  auto readByte(Address addr) -> u8;
  auto readWord(Address addr) -> u16;

  auto writeByte(Address addr, u8 data) -> void;
  auto writeWord(Address addr, u16 data) -> void;

private:
  std::vector<DeviceMemoryMap::Ptr> devices_;
};

}
