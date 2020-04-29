#pragma once

#include <types.h>
#include <util/natural.h>
#include <util/bit.h>

namespace brgb::sm83 {

// Encoded in the Opcode's 'y' field
enum class AluOp {
  Add = 0, Adc = 1, Sub = 2, Sbc = 3, And = 4, Xor = 5, Or = 6, Cp = 7,
};

// Encoded in the Opcode's 'y' field
enum class RotOp {
  Rlc = 0, Rrc = 1, Rl = 2, Rr = 3, Sla = 4, Sra = 5, Swap = 6, Srl = 7,
};

enum class AkkuOp {
  Rlca = 0, Rrca = 1, Rla = 2, Rra = 3,
  Daa = 4, Cpl = 5, Scf = 6, Ccf = 7,
};

enum class Reg8 {
  b = 0, c = 1, d = 2, e = 3, h = 4, l = 5, HLIndirect = 6, a = 7,
};

enum class Reg16_rp {
  bc = 0, de = 1, hl = 2, sp = 3,
};

enum class Reg16_rp2 {
  bc = 0, de = 1, hl = 2, af = 3,
};

enum class ConditionCode {
  nz = 0, z = 1, nc = 2, c = 3,
};

class Instruction {
public:
  using Opcode = Natural<8>;

  Instruction(u8 op);

  // Opcode structure:
  //   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  //     ^   ^   ^   ^   ^   ^       ^
  //     |   |   |   |   |   |       |
  //     |   |   |   |   |   |       |
  //     \___/   \___/   v   \_______/
  //       x     | p     q       z
  //             |       |
  //             \_______/
  //                 y

  inline auto opcode() -> u8 { return op_; }

  inline auto x() { return op_.bit(6, 7); }
  inline auto y() { return op_.bit(3, 5); }
  inline auto z() { return op_.bit(0, 2); }

  inline auto p() { return op_.bit(4, 5); }
  inline auto q() { return op_.bit(3); }

  inline auto reg8_y() -> Reg8 { return (Reg8)y().get(); }
  inline auto reg8_z() -> Reg8 { return (Reg8)z().get(); }

  inline auto reg16rp_p() -> Reg16_rp { return (Reg16_rp)p().get(); }
  inline auto reg16rp2_p() -> Reg16_rp2 { return (Reg16_rp2)p().get(); }

  inline auto cc() -> ConditionCode { return (ConditionCode)y().get(); }
  inline auto ccForJr() -> ConditionCode { return (ConditionCode)(y().get()-4); }

  inline auto alu_y() -> AluOp { return (AluOp)y().get(); }
  inline auto rot_y() -> RotOp { return (RotOp)y().get(); }
  inline auto akku_y() -> AkkuOp { return (AkkuOp)y().get(); }

  // x=0
  //   z=0  =>  y=0(nop), y=1(ld (a16), sp), y=2(stop), y=3(jr), y=4..7(jr cc)
  //   z=1  =>  q=0(ld reg16rp, imm16) q=1(add hl, reg16rp)
  //   z=2  =>  q=0 [
  //              p=0(ld (bc), a) p=1(ld (de), a) p=2(ld (hl+), a) p=3(ld (hl-), a)
  //            ]
  //            q=1 [
  //              p=0(ld a, (bc)) p=1(ld a, (de)) p=2(ld a, (hl+)) p=3(ld a, (hl-))
  //            ]
  //   z=3  =>  q=0(inc reg16rp) q=1(dec reg16rp)
  //   z=4  =>  inc reg8
  //   z=5  =>  dec re8
  //   z=6  =>  ld reg8, imm8
  //   z=7  =>  AkkuOp[y]
  //
  // x=1
  //   ld reg8[y], reg8[z]
  //
  //   z=6  =>  y=6(halt)
  //
  // x=2
  //   AluOp[y] reg8[z]
  //
  // x=3
  //   z=0  =>  y=0..3(ret cc)
  //            y=4(ldh ($FF00+imm8), a)  y=5(add sp, imm8)
  //            y=6(ldh a, ($FF00+imm8))  y=7(ld hl, sp+imm8)
  //   z=1  =>  q=0 [ pop reg16rp2 ]
  //            q=1 [
  //              p=0(ret) p=1(reti) p=2(jp (hl)) p=3(ld sp, hl)
  //            ]
  //   z=2  =>  y=0..3(jp cc, addr16)
  //            y=4(ldh a, ($FF00+c)) y=5(ld (addr16), a)
  //            y=6(ldh ($FF00+c), a) y=7(ld a, (addr16))
  //   z=3  =>  y=0(jp addr16) y=1(CB prefix) y=2(ILLEGAL) y=3(ILLEGAL)
  //            y=4(ILLEGAL)   y=5(ILLEGAL)   y=6(di)      y=7(ei)
  //   z=4  =>  y=0..3(call cc, addr16)
  //            y=4(ILLEGAL) y=5(ILLEGAL) y=6(ILLEGAL) y=7(ILLEGAL)
  //   z=5  =>  q=0 [ push reg16rp2 ]
  //            q=1 [
  //              p=0(call addr16) p=1(ILLEGAL) p=2(ILLEGAL) p=3(ILLEGAL)
  //            ]
  //   z=6  =>  AluOp[y] imm8
  //   z=7  =>  rst


private:
  Natural<8> op_;
};

}
