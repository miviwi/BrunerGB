#include <bus/memorymap.h>

namespace brgb {

template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::readByte(Address addr) -> u8
{
  return 0;
}

template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::readWord(Address addr) -> u16
{
  return 0;
}

template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::writeByte(Address addr, u8 data) -> void
{
}

template <size_t AddressWidth>
auto AddressSpace<AddressWidth>::writeWord(Address addr, u16 data) -> void
{
}

template class AddressSpace<16>;

}
