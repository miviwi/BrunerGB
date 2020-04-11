#pragma once

#include <types.h>

#include <optional>

namespace brgb {

class BusReadHandler {
};

class BusWriteHandler {
};

template <typename Address>
class MappedAddressRange {
public:

  auto lookupR(Address addr) -> BusReadHandler&;
  auto lookupW(Address addr) -> BusWriteHandler&;

private:
};

}
