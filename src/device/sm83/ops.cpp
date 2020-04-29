#include <device/sm83/cpu.h>

#include <cassert>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define OP_STUB assert(0 && STRINGIFY(__PRETTY_FUNCTION__) " stub!");

namespace brgb::sm83 {

auto Processor::opHALT() -> void
{
  OP_STUB;
}

}
