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

  enum : Address {
    AddressRangeInvalid = (Address)~0ull,
  };

  Address mask; // Must be applied to addresses before
                //   passing them into read/write

  Address lo, hi;

  BusReadHandler read;
  BusWriteHandler write;
};

}
