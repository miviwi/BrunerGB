#include <device/sm83/cpu.h>
#include <device/sm83/instruction.h>

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

auto Processor::opcode() -> u8
{
  return read(PC++);
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

auto Processor::reg(Reg8 which) -> u8
{
  switch(which) {
  case Reg8::b:          return B;
  case Reg8::c:          return C;
  case Reg8::d:          return D;
  case Reg8::e:          return E;
  case Reg8::h:          return H;
  case Reg8::l:          return L;
  case Reg8::HLIndirect: return read(HL);
  case Reg8::a:          return A;
  }

  assert(0);   // Unreachable
  return 0;
}

auto Processor::reg(Reg8 which, u8 data) -> void
{
  switch(which) {
  case Reg8::b:          B = data; break;
  case Reg8::c:          C = data; break;
  case Reg8::d:          D = data; break;
  case Reg8::e:          E = data; break;
  case Reg8::h:          H = data; break;
  case Reg8::l:          L = data; break;
  case Reg8::HLIndirect: write(HL, data); break;
  case Reg8::a:          A = data; break;
  }
}

auto Processor::reg(Reg16_rp which) -> u16
{
  switch(which) {
  case Reg16_rp::bc: return BC;
  case Reg16_rp::de: return DE;
  case Reg16_rp::hl: return HL;
  case Reg16_rp::sp: return SP;
  }

  assert(0);   // Unreachable
  return 0;
}

auto Processor::reg(Reg16_rp which, u16 data) -> void
{
  switch(which) {
  case Reg16_rp::bc: BC = data; break;
  case Reg16_rp::de: DE = data; break;
  case Reg16_rp::hl: HL = data; break;
  case Reg16_rp::sp: SP = data; break;
  }
}

auto Processor::reg(Reg16_rp2 which) -> u16
{
  switch(which) {
  case Reg16_rp2::bc: return BC;
  case Reg16_rp2::de: return DE;
  case Reg16_rp2::hl: return HL;
  case Reg16_rp2::af: return AF;
  }

  assert(0);   // Unreachable
  return 0;
}

auto Processor::reg(Reg16_rp2 which, u16 data) -> void
{
  switch(which) {
  case Reg16_rp2::bc: BC = data; break;
  case Reg16_rp2::de: DE = data; break;
  case Reg16_rp2::hl: HL = data; break;
  case Reg16_rp2::af: AF = data; break;
  }
}

auto Processor::alu(AluOp op, u8 val) -> void
{
}

auto Processor::rot(RotOp op, u8 val) -> void
{
}

auto Processor::akku(AkkuOp op) -> void
{
}

auto Processor::instruction() -> void
{
  auto i = Instruction(opcode());

  auto op_x0 = [this](Instruction i) {
    switch(i.z()) {
    case 0: switch(i.y()) {
      case 0: /* nop */ break;
    }
    break;

    case 1:
      if(i.q() == 0) {
        //  ld <reg16>, imm16
        reg(i.reg16rp_p(), operand16());
      } else {
      }
      break;

    //  ld <reg8>, imm8
    case 6: reg(i.reg8_y(), operand8()); break;
    }
  };

  auto op_x3 = [this](Instruction i) {
    switch(i.z()) {
    }
  };

  switch(i.x()) {
  case 0: op_x0(i); break;

  //  ld <reg8>, <reg8>
  //  halt
  case 1: {
    u8 y = i.y();
    u8 z = i.z();

    if(z == 6 && y == 6) return opHALT();

    u8 data = reg(i.reg8_z());
    reg(i.reg8_y(), data);
    break;
  }

  //  add a, <reg8>
  //  adc a, <reg8>
  //  sub <reg8>
  //  sbc a, <reg8>
  //  and <reg8>
  //  xor <reg8>
  //  or <reg8>
  //  cp <reg8>
  case 2: alu(i.alu_y(), reg(i.reg8_z())); break;

  case 3: op_x3(i); break;
  }
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
