#include <device/sm83/cpu.h>

#include <cassert>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define OP_STUB assert(0 && STRINGIFY(__PRETTY_FUNCTION__) " stub!");

namespace brgb::sm83 {

auto Processor::op_nop() -> void
{
  OP_STUB;
}

auto Processor::op_stop() -> void
{
  OP_STUB;
}

auto Processor::op_jr(u1 cond) -> void
{
  OP_STUB;
}

auto Processor::op_ld_Imm16(Natural<16>& reg) -> void
{
  OP_STUB;
}

auto Processor::op_ld_Indirect16(Natural<16>& ptr, u8 data) -> void
{
  OP_STUB;
}

auto Processor::op_ldi(u8 data) -> void
{
  OP_STUB;
}

auto Processor::op_ldd(u8 data) -> void
{
  OP_STUB;
}

auto Processor::op_inc_Reg16(Natural<16>& reg) -> void
{
  OP_STUB;
}

auto Processor::op_dec_Reg16(Natural<16>& reg) -> void
{
  OP_STUB;
}

}
