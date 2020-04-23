#include <device/sm83/cpu.h>

#include <bus/memorymap.h>

#define A r.a
#define F r.f
#define B r.b
#define C r.c
#define D r.d
#define E r.e
#define H r.h
#define L r.l

#define ZF r.flags.z
#define NF r.flags.n
#define HF r.flags.h
#define CF r.flags.c

#define AF r.af
#define BC r.bc
#define DE r.de
#define HL r.hl

#define PC r.pc
#define SP r.sp

namespace brgb::sm83 {

auto Processor::connect(SystemBus *sys_bus) -> void
{
  bus_.reset(ProcessorBus::for_device(sys_bus, this));
}

auto Processor::bus() -> ProcessorBus&
{
  return *bus_;
}

auto Processor::power() -> void
{
  r = { };
}

auto Processor::operand8() -> u8
{
  return read(PC++);
}

auto Processor::operand16() -> u16
{
  Natural<16> operand;

  operand.bit(0, 7)  = read(PC++);
  operand.bit(8, 15) = read(PC++);

  return operand;
}

auto Processor::push16(u16 data) -> void
{
  write(--SP, data >> 8);
  write(--SP, data >> 0);
}

auto Processor::pop16() -> u16
{
  Natural<16> data;

  data.bit(0, 7)  = read(SP++);
  data.bit(8, 15) = read(SP++);

  return data;
}

}

#undef SP
#undef PC

#undef HL
#undef DE
#undef BC
#undef AF

#undef L
#undef H
#undef E
#undef D
#undef C
#undef B
#undef F
#undef A
