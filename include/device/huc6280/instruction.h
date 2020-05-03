#pragma once

#include <types.h>
#include <util/natural.h>
#include <util/bit.h>

namespace brgb::huc6280 {

struct Op_c01 {
  enum {
    // Instruction's 'a' bits
    Ora = 0b000,
    And = 0b001,
    Eor = 0b010,
    Adc = 0b011,
    Sta = 0b100,
    Lda = 0b101,
    Cmp = 0b110,
    Sbc = 0b111,
  };
};

struct AddrMode_c01 {
  enum {
    // Instruction's 'b' bits
    ZpInd_X = 0b000,
    Zp      = 0b001,
    Imm     = 0b010,
    Abs     = 0b011,
    ZpInd_Y = 0b100,
    Zp_X    = 0b101,
    Abs_X   = 0b110,
    Abs_Y   = 0b111,
  };
};

struct Op_c10 {
  enum {
    // Instruction's 'a' bits
    Asl = 0b000,
    Rol = 0b001,
    Lsr = 0b010,
    Ror = 0b011,
    Stx = 0b100,
    Ldx = 0b101,
    Dec = 0b110,
    Inc = 0b111,
  };
};

struct AddrMode_c10 {
  enum {
    // Instruction's 'b' bits
    Imm = 0b000,
    Zp  = 0b001,
    Acc = 0b010,
    Abs = 0b011,

    // For LDX/STX use Zp_Y addressing mode
    // For LDY/STY use Zp_X,Abs_X addressing mode
    Zp_X__Zp_Y   = 0b101,
    Abs_X__Abs_Y = 0b111,
  };
};

struct Op_c00 {
  enum {
    // Instruction's 'a' bits
    Tsb = 0b000,
    Bit = 0b001,
    Jmp = 0b010,
    Jmp_Ind = 0b011,
    Sty = 0b100,
    Ldy = 0b101,
    Cpy = 0b110,
    Cpx = 0b111,
  };
};

struct Addrmode_c00 {
  enum {
    // Instruction's 'b' bits
    Imm   = 0b000,
    Zp    = 0b001,
    Abs   = 0b011,
    Zp_X  = 0b101,
    Abs_X = 0b111,
  };
};

struct ConditionCode {
  enum {
    N = 0b00,
    V = 0b01,
    C = 0b10,
    Z = 0b11,
  };
};

class Instruction {
public:
  Instruction(u8 op);

  // Opcode structure:
  //   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  //     ^   ^   ^   ^       ^   ^   ^
  //     |   |   |   |       |   |   |
  //     |   |   |   |       |   |   |
  //     \___/   v   \_______/   \___/
  //     | x     y       b         c
  //     |       |
  //     \_______/
  //         a

  inline auto a() { return op_.bit(5, 7); }
  inline auto b() { return op_.bit(2, 4); }
  inline auto c() { return op_.bit(0, 1); }

  // Condition code bits
  inline auto x() { return op_.bit(6, 7); }
  // Condition compare bit
  inline auto y() { return op_.bit(5); }

private:
  Natural<8> op_;
};

}
