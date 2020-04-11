#pragma once

#include <types.h>

#include <utility>

namespace brgb {

class BusReadHandler {
};

class BusWriteHandler {
};

struct MappedAddressRange {
  using Address = u64;
  using Range = std::pair<Address /* lo */, Address /* hi */>;

  enum : Address {
    AddressRangeInvalid = (Address)~0ull,
  };

  const char *address_range;   // For debug purposes

  Address mask; // Must be applied to addresses before
                //   passing them into read/write

  Range *ranges;  // Points into DeviceMemoryMap.ranges_,
                  //   stores the appropriate Range's
                  //   i.e. the ones at which read/write
                  //   should respond and is terminated by
                  //     Range{ AddressRangeInvalid, AddressRangeInvalid }

  BusReadHandler read;
  BusWriteHandler write;
};

}
