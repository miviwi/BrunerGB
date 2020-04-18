#include <device/lr35902/disassembler.h>

#include <utility>
#include <string>
#include <sstream>
#include <unordered_map>

#include <cassert>
#include <cstddef>

namespace brgb::lr35902 {

static constexpr u8 OpcodeNOP  = 0x00;
static constexpr u8 OpcodeSTOP = 0x10;
static constexpr u8 OpcodeHALT = 0x76;
static constexpr u8 OpcodeRLCA = 0x07;
static constexpr u8 OpcodeRLA  = 0x17;
static constexpr u8 OpcodeDAA  = 0x27;
static constexpr u8 OpcodeSCF  = 0x37;
static constexpr u8 OpcodeRRCA = 0x0F;
static constexpr u8 OpcodeRRA  = 0x1F;
static constexpr u8 OpcodeCPL  = 0x2F;
static constexpr u8 OpcodeCCF  = 0x3F;

static constexpr u8 OpcodeJP_Always          = 0xC3;
static constexpr u8 OpcodeJP_Always_Indirect = 0xE9;
static constexpr u8 OpcodeJR_Always          = 0x18;
static constexpr u8 OpcodeCALL_Always        = 0xCD;
static constexpr u8 OpcodeRET_Always         = 0xC9;

static constexpr u8 OpcodeADD_SP_Imm = 0xE8;

static const std::unordered_map<OpcodeMnemonic, std::string> p_op_mnem_to_str = {
  { op_Invalid, "<invalid>" },

  { op_nop, "nop" },
  { op_stop, "stop" }, { op_halt, "halt" },
  { op_jp, "jp" }, { op_jr, "jr" },
  { op_ld, "ld" },
  { op_inc, "inc" }, { op_dec, "dec" },
  { op_rlca, "rlca" }, { op_rla, "rla" }, { op_rrca, "rrca" }, { op_rra, "rra" },
  { op_daa, "daa" },
  { op_cpl, "cpl" },
  { op_scf, "scf" }, { op_ccf, "ccf" },
  { op_add, "add" }, { op_adc, "adc" }, { op_sub, "sub" }, { op_sbc, "sbc" },
  { op_and, "and" }, { op_or, "or" }, { op_xor, "xor" },
  { op_cp, "cp" },
  { op_call, "call" },
  { op_ret, "ret" }, { op_reti, "reti" },
  { op_push, "push" }, { op_pop, "pop" },
  { op_ei, "ei" }, { op_di, "di" },
  { op_rst, "rst" },
};

static const std::unordered_map<OpcodeMnemonic, std::string> p_op_0xCB_mnem_to_str = {
  { op_rlc, "rlc" }, { op_rl, "rl" }, { op_rrc, "rrc" }, { op_rr, "rr" },
  { op_sla, "sla" }, { op_sra, "sra" }, { op_srl, "srl" },
  { op_swap, "swap" },
  { op_bit, "bit" }, { op_res, "res" }, { op_set, "set" },
};

inline auto lo_nibble(u8 op) -> u8 { return op & 0x0F; }
inline auto hi_nibble(u8 op) -> u8 { return op & 0xF0; }

template <typename... Args>
inline auto hi_nibble_matches(u8 op, Args... args) -> bool
{
  return ((hi_nibble(op) == args) || ...);
}

inline auto hi_nibble_between(u8 op, u8 min, u8 max) -> bool
{
  return hi_nibble(op) >= min && hi_nibble(op) <= max;
}

template <typename... Args>
inline auto lo_nibble_matches(u8 op, Args... args) -> bool
{
  return ((lo_nibble(op) == args) || ...);
}

inline auto lo_nibble_between(u8 op, u8 min, u8 max) -> bool
{
  return lo_nibble(op) >= min && lo_nibble(op) <= max;
}

auto Instruction::OperandReg_to_str(OperandReg reg) -> std::string
{
  switch(reg) {
  // <reg8>
  case RegA: return "a";
  case RegF: return "f";
  case RegB: return "b";
  case RegC: return "c";
  case RegD: return "d";
  case RegE: return "e";
  case RegH: return "h";
  case RegL: return "l";

  // <reg16>
  case RegAF: return "af";
  case RegBC: return "bc";
  case RegDE: return "de";
  case RegHL: return "hl";
  case RegSP: return "sp";

  case RegBCInd: return "(bc)";
  case RegDEInd: return "(de)";
  case RegHLInd: return "(hl)";

  case RegHLIInd: return "(hl+)";
  case RegHLDInd: return "(hl-)";
  }

  return "<invalid>";
}

auto Instruction::OperandCondition_to_str(OperandCondition cond) -> std::string
{
  switch(cond) {
  case ConditionC:  return "c";
  case ConditionNC: return "nc";
  case ConditionZ:  return "z";
  case ConditionNZ: return "nz";
  }

  return "<invalid>";
}

auto Instruction::op_mnem_to_str(OpcodeMnemonic op) -> std::string
{
  auto it = p_op_mnem_to_str.find(op);

  return it != p_op_mnem_to_str.end() ? it->second : "<unknown>";
}

auto Instruction::op_0xCB_mnem_to_str(OpcodeMnemonic op) -> std::string
{
  auto it = p_op_0xCB_mnem_to_str.find(op);

  return it != p_op_0xCB_mnem_to_str.end() ? it->second : "<unknown>";
}

Instruction::Instruction(u8 *mem) :
  mem_(mem)
{
}

auto Instruction::disassemble(u8 *ptr) -> u8 *
{
  // Store the Instruction's offset
  offset_ = ptr - mem_;

  // Fetch the opcode
  u8 op = *ptr++;
  op_ = op;

  // Delegate to alternate method for CB-prefixed opcodes
  if(op == CB_prefix) return disassembleCBPrefixed(ptr);

  if(hi_nibble_between(op, 0x00, 0x30)) {
    return disassemble0x00_0x30(ptr);
  } else if(hi_nibble_between(op, 0x40, 0x70)) {
    if(op == OpcodeHALT) {  // The HALT opcode is mixed-in with the LDs
      op_mnem_ = op_halt;
    } else {
      op_mnem_ = op_ld;
    }
  } else if(hi_nibble_between(op, 0x80, 0xB0)) {
    return disassemble0x80_0xB0(ptr);
  } else if(hi_nibble_between(op, 0xC0, 0xF0)) {
    return disassemble0xC0_0xF0(ptr);
  }

  return ptr;
}

auto Instruction::numOperands() -> unsigned
{
  u8 op = op_;

  switch(op_mnem_) {
  case op_nop:  return 0;
  case op_stop: return 1;   // Always 0
  case op_halt: return 0;

  case op_jp:
  case op_jr:
  case op_call:
  case op_ret:
  case op_reti:
    // For these instructions the accumulator ('A' register)
    //   is listed as an operand instead of being implied
   if(op == OpcodeJP_Always || op == OpcodeJP_Always_Indirect || op == OpcodeJR_Always
     || op == OpcodeCALL_Always || op == OpcodeRET_Always) {
      return 1;
    }

    return 2;

  case op_ld: return 2;

  case op_inc:
  case op_dec: return 1;

  case op_rlca:
  case op_rla:
  case op_rrca:
  case op_rra: return 0;   // operand is implied

  case op_daa:
  case op_cpl: return 0;

  case op_scf:
  case op_ccf: return 0;

  case op_add:
  case op_adc:
  case op_sbc: return 2;   // the 'A' register operand is explicit

  case op_sub:
  case op_and:
  case op_xor:
  case op_or:
  case op_cp: return 1;

  case op_push:
  case op_pop: return 1;

  case op_ei:
  case op_di: return 0;

  case op_rst: return 1;

  case op_rlc:
  case op_rl:
  case op_rrc:
  case op_rr:
  case op_sla:
  case op_sra:
  case op_srl: return 1;

  case op_swap: return 1;

  case op_bit:
  case op_res:
  case op_set: return 2;
  }

  return ~0u;
}

auto Instruction::operandType(unsigned which) -> OperandType
{
  assert(which < 2 && "Instruction::operandType() called with out-of-range 'which'!");

  u8 op = op_;

  switch(op_mnem_) {
  case op_nop:
  case op_halt: return OperandNone;

  case op_stop: return OperandImm8;   // Always 0

  case op_daa:
  case op_cpl:
  case op_scf:
  case op_ccf:
  case op_rlca:
  case op_rla:
  case op_rrca:
  case op_rra:
  case op_ei:
  case op_di: return OperandImplied;

  case op_push:
  case op_pop: return OperandReg16;

  case op_reti: return OperandImplied;

  case op_rst: return OperandRSTVector;
  }

  if(op_mnem_ == op_jp || op_mnem_ == op_jr || op_mnem_ == op_call || op_mnem_ == op_ret) {
    if(op == OpcodeJP_Always || op == OpcodeCALL_Always) {
      return OperandAddress16;
    } else if(op == OpcodeJR_Always) {
      return OperandRelOffset8;
    } else if(op == OpcodeRET_Always) {
      return OperandImplied;
    } else if(op == OpcodeJP_Always_Indirect) {
      return OperandReg16Indirect;
    }

    switch(op_mnem_) {
    case op_jr: return which == 0 ? OperandCond : OperandRelOffset8;

    default:    return which == 0 ? OperandCond : OperandAddress16;
    }
  } else if(op_mnem_ == op_inc || op_mnem_ == op_dec) {
    if(lo_nibble_matches(op, 0x04, 0x05, 0x0C, 0x0D)) {
      if(hi_nibble_matches(op, 0x30)) return OperandReg16Indirect;   // inc/dec (hl)

      return OperandReg8;
    } else if(lo_nibble_matches(op, 0x03, 0x0B)) {
      return OperandReg16;
    }
  } else if(hi_nibble_between(op, 0x00, 0x30)) {
    if(lo_nibble_matches(op, 0x01)) {
      return which == 0 ? OperandReg16 : OperandImm16;
    } else if(lo_nibble_matches(op, 0x02)) {
      return which == 0 ? OperandReg16Indirect : OperandReg8;
    } else if(lo_nibble_matches(op, 0x0A)) {
      return which == 0 ? OperandReg8 : OperandReg16Indirect;
    }
  } else if(hi_nibble_between(op, 0x40, 0x70)) {
    //  ld <reg8>, <reg8>
    //  ld (hl), <reg8>
    //  ld <reg8>, (hl)

    if(lo_nibble_matches(op, 0x06) || lo_nibble_matches(op, 0x0E)) {
      switch(which) {
      case 1: return OperandReg16Indirect;
      }
    } else if(hi_nibble_matches(op, 0x70) && lo_nibble_between(op, 0x00, 0x07)) {
      switch(which) {
      case 0: return OperandReg16Indirect;
      }
    } else if(lo_nibble_matches(op, 0x02)) {
      return which == 0 ? OperandReg16Indirect : OperandReg8;
    } else if(lo_nibble_matches(op, 0x0A)) {
      return which == 0 ? OperandReg8 : OperandReg16Indirect;
    } else if(lo_nibble_matches(op, 0x08)) {
      return which == 0 ? OperandPtr16 : OperandReg16;
    } else if(lo_nibble_matches(op, 0x09)) {
      // add hl, <reg16>
      return OperandReg16;
    } 

    return OperandReg8;
  } else if(hi_nibble_between(op, 0x80, 0xB0)) {
    //  add a, <reg8>/(hl)
    //  adc a, <reg8>/(hl)
    //  sub <reg8>/(hl)
    //  sbc a, <reg8>/(hl)
    //  and <reg8>/(hl)
    //  xor <reg8>/(hl)
    //  or <reg8>/(hl)
    //  cp <reg8>/(hl)

    if(lo_nibble_matches(op, 0x06, 0x0E)) {
      if(op_mnem_ == op_add || op_mnem_ == op_adc || op_mnem_ == op_sbc) {
        return which == 1 ? OperandReg16Indirect : OperandReg8;
      }
    }

    return OperandReg8;
  } else if(hi_nibble_between(op, 0xC0, 0xF0)) {
    //   add a, <imm8>
    //   adc a, <imm8>
    //   sub <imm8>
    //   sbc a, <imm8>
    //   and <imm8>
    //   xor <imm8>
    //   or <imm8>
    //   cp <imm8>
    if(lo_nibble_matches(op, 0x06, 0x0E)) {
      if(op_mnem_ == op_add || op_mnem_ == op_adc || op_mnem_ == op_sbc) {
        return which == 0 ? OperandReg8 : OperandImm8;
      }

      return OperandImm8;
    } else if(op == 0xEA /* ld (<addr16>), a */) {
      return which == 0 ? OperandPtr16 : OperandReg8;
    } else if(op == 0xFA /* ld a, (<addr16>) */) {
      return which == 0 ? OperandReg8 : OperandPtr16;
    }
  }

  return OperandInvalid;
}

auto Instruction::reg(unsigned which) -> OperandReg
{
  assert(which < 2 && "Instruction::reg() called with out-of-range 'which'!");

  static const OperandReg idx_to_reg8[] = {
    RegB, RegC, RegD, RegE, RegH, RegL, RegHLInd, RegA,
  };

  static const OperandReg idx_to_reg16[] = {
    // inc <reg16>
    // dec <reg16>
    // ld <reg16>,<imm16>
    // add hl,<reg16>
    RegBC, RegDE, RegHL, RegSP,

    // push <reg16>
    // pop <reg16>
    RegBC, RegDE, RegHL, RegAF,
  };

  u8 op = op_;

  if(hi_nibble_between(op_, 0x00, 0x30)) {
    if(lo_nibble_matches(op_, 0x06, 0x0E) && which == 0) {          // ld <reg8>, <imm8>
      /*
        0x06 - 0000 0110   // ld b, <imm8>
        0x0E - 0000 1110   // ld c, <imm8>
        0x16 - 0001 0110   // ld d, <imm8>
        0x1E - 0001 1110   // ld e, <imm8>
        0x26 - 0010 0110   // ld h, <imm8>
        0x2E - 0010 1110   // ld l, <imm8>
        0x36 - 0011 0110   // ld (hl), <imm8>
        0x3E - 0011 1110   // ld a, <imm8>
      */

      return idx_to_reg16[op_.bit(3, 5)];
    } else if(lo_nibble_matches(op_, 0x01, 0x09) && which == 0) {   // ld <reg16>, <imm16>
      /*                                                            // add hl, <reg16>
        0x01 - 0000 0001   // ld bc, <imm16>
        0x11 - 0001 0001   // ld de, <imm16>
        0x21 - 0010 0001   // ld hl, <imm16>
        0x31 - 0011 0001   // ld sp, <imm16>
      */

      return idx_to_reg16[op_.bit(4, 5)];
    } else if(lo_nibble_matches(op_, 0x02)) {                       // ld (bc), a
      if(which == 1) return RegA;                                   // ld (de), a
                                                                    // ld (hl+), a
      // which == 0                                                 // ld (hl-), a
      switch(op) {
      case 0x02: return RegBCInd;
      case 0x12: return RegDEInd;
      case 0x22: return RegHLIInd;
      case 0x32: return RegHLDInd;
      }
    } else if(lo_nibble_matches(op_, 0x0A)) {                       // ld a, (bc)
      if(which == 0) return RegA;                                   // ld a, (de)
                                                                    // ld a, (hl+)
      // which == 1                                                 // ld a, (hl-)
      switch(op) {
      case 0x0A: return RegBCInd;
      case 0x1A: return RegDEInd;
      case 0x2A: return RegHLIInd;
      case 0x3A: return RegHLDInd;
      }
    }
  } else if(hi_nibble_between(op, 0x40, 0x70)) {     // ld <reg8>, <reg8>
    if(op_mnem_ == op_halt) return RegInvalid;    // HALT doesn't have any operands

    u8 src_idx = op_.bit(0, 2);
    u8 dst_idx = op_.bit(3, 5);

    switch(which) {
    case 0: return idx_to_reg8[dst_idx];
    case 1: return idx_to_reg8[src_idx];
    }
  } else if(hi_nibble_between(op, 0x80, 0xB0)) {
    // The accumulator is explicit in these instructions...
    if(op_mnem_ == op_add || op_mnem_ == op_adc || op_sbc) {
      if(which == 0) return RegA;

      // Unify codepaths (which == 1 here)
      which = 0;
    }

    // ...but for all of them the encoding is the same
    return which == 0 ? idx_to_reg8[op_.bit(0, 2)] : RegInvalid;
  } else if(op_mnem_ == op_inc || op_mnem_ == op_dec && which == 0) {
    if(lo_nibble_matches(op_, 0x04, 0x05, 0x0C, 0x0D)) {    // inc/dec <reg8>
      /*
        0x04 - 0000 0100   // inc b
        0x0C - 0000 1100   // inc c
        0x14 - 0001 0100   // inc d
        0x1C - 0001 1100   // inc e
        0x24 - 0010 0100   // inc h
        0x2C - 0010 1100   // inc l
        0x34 - 0011 0100   // inc (hl)
        0x3C - 0011 1100   // inc a
      */

      return idx_to_reg8[op_.bit(3, 5)];
    } else if(lo_nibble_matches(op_, 0x03, 0x0B)) {         // inc/dec <reg16>
      /*
        0x03 - 0000 0011   // inc bc
        0x13 - 0001 0011   // inc de
        0x23 - 0010 0011   // inc hl
        0x33 - 0011 0011   // inc sp
      */

      return idx_to_reg16[op_.bit(4, 5)];
    }
  } else if(op_mnem_ == op_push || op_mnem_ == op_pop && which == 0) {
    /*
      0xC1 - 1100 0001  // pop bc
      0xD1 - 1101 0001  // pop de
      0xE1 - 1110 0001  // pop hl
      0xF1 - 1111 0001  // pop af
      0xC5 - 1100 0101  // push bc
      0xD5 - 1101 0101  // push de
      0xE5 - 1110 0101  // push hl
      0xF5 - 1111 0101  // push af
    */

    return idx_to_reg16[op_.bit(4, 5) + 4];
  } else if(op == 0xEA /* ld (<addr16>), a */) {
    if(which == 1) return RegA;
  } else if(op == 0xFA /* ld a, (<addr16>) */) {
    if(which == 0) return RegA;
  }

  return RegInvalid;
}

auto Instruction::imm8() -> u8
{
  return operand_lo_.get();
}

auto Instruction::imm16() -> u16
{
  return operand_.get();
}

auto Instruction::address() -> u16
{
  return operand_.get();
}

auto Instruction::relOffset() -> i8
{
  return (i8)operand_lo_.get();
}

auto Instruction::cond() -> OperandCondition
{
  static const OperandCondition cc_to_condition[] = {
    ConditionNZ, ConditionZ, ConditionNC, ConditionC,
  };

  u8 op = op_;

  // Only JP,JR,CALL,RET opcodes have an OperandCondition operand
  if(op_mnem_ != op_jp && op_mnem_ != op_jr && op_mnem_ != op_call && op_mnem_ != op_ret) {
    return ConditionInvalid;
  }

  // Handle unconditional JP,JR,CALL,RET opcodes
  if(op == OpcodeJP_Always || op == OpcodeJP_Always_Indirect || op == OpcodeJR_Always || 
      op == OpcodeCALL_Always || op == OpcodeRET_Always) {
    return ConditionInvalid;
  }

  /*
    0x20 - 0010 0000  // nz
    0xC0 - 1100 0000  // nz
    0xC2 - 1100 0010  // nz
    0xC4 - 1100 0100  // nz
    0xCA - 1100 1010  //  z
    0x28 - 0010 1000  //  z
    0xC8 - 1100 1000  //  z
    0xCC - 1100 1100  //  z
    0x30 - 0011 0000  // nc
    0xD0 - 1101 0000  // nc
    0xD2 - 1101 0010  // nc
    0xD4 - 1101 0100  // nc
    0x38 - 0011 1000  //  c
    0xD8 - 1101 1000  //  c
    0xDA - 1101 1010  //  c
    0xDC - 1101 1100  //  c
  */

  return cc_to_condition[op_.bit(3, 4)];
}

auto Instruction::RSTVector() -> u8
{
  if(op_mnem_ != op_rst) return RSTVectorInvalid;

  /*
    0xC7 - 1100 0111  // 00h
    0xCF - 1100 1111  // 08h
    0xD7 - 1101 0111  // 10h
    0xDF - 1101 1111  // 18h
    0xE7 - 1110 0111  // 20h
    0xEF - 1110 1111  // 28h
    0xF7 - 1111 0111  // 30h
    0xFF - 1111 1111  // 38h
  */

  return op_.bit(3, 5).get() * 0x8;
}

auto Instruction::toStr() -> std::string
{
  auto op = opcodeToStr();

  // Right-pad the mnemonic with spaces to a width of 4
  while(op.length() < 4) op += ' ';

  auto operands = operandsToStr();

  return util::fmt("%s %s", op.data(), operands.data());
}

auto Instruction::disassemble0x00_0x30(u8 *ptr) -> u8 *
{
  dispatchOnLoNibble(
      std::make_pair((u8)0x00, [this,&ptr]() { dispatchOnHiNibble(
        //  nop
        std::make_pair(OpcodeNOP, [this,&ptr]() {
          op_mnem_ = op_nop;
        }),

        //  stop
        std::make_pair(OpcodeSTOP, [this,&ptr]() {
          op_mnem_ = op_stop;

          // STOP has a dummy 0x00 operand
          operand_ = *ptr++;
        }),

        //  jr nz, <r8>
        std::make_pair((u8)0x20, [this,&ptr]() {
          op_mnem_ = op_jr;

          // Fetch the jump displacement
          operand_ = *ptr++;
        }),
        //  jr nc, <r8>
        std::make_pair((u8)0x30, [this,&ptr]() {
          op_mnem_ = op_jr;

          // Fetch the jump displacement
          operand_ = *ptr++;
        }));
      }),

      //  ld bc, <imm16>
      //  ld de, <imm16>
      //  ld hl, <imm16>
      //  ld sp, <imm16>
      std::make_pair((u8)0x01, [this,&ptr]() {
        op_mnem_ = op_ld;

        // Fetch the source address
        //   - Little-endian byte ordering
        operand_lo_ = *ptr++;
        operand_hi_ = *ptr++;
      }),
      //  ld (bc), a
      //  ld (de), a
      //  ld (hl+), a
      //  ld (hl-), a
      std::make_pair((u8)0x02, [this,&ptr]() {
        op_mnem_ = op_ld;
      }),

      //  inc bc
      //  inc de
      //  inc hl
      //  inc sp
      std::make_pair((u8)0x03, [this,&ptr]() {
        op_mnem_ = op_inc;
      }),

      //  dec bc
      //  dec de
      //  dec hl
      //  dec sp
      std::make_pair((u8)0x0B, [this,&ptr]() {
        op_mnem_ = op_dec;
      }),

      //  inc b
      //  inc d
      //  inc h
      //  inc (hl)
      std::make_pair((u8)0x04, [this,&ptr]() {
        op_mnem_ = op_inc;
      }),
      //  inc c
      //  inc e
      //  inc l
      //  inc a
      std::make_pair((u8)0x0C, [this,&ptr]() {
        op_mnem_ = op_inc;
      }),

      //  dec b
      //  dec d
      //  dec h
      //  dec (hl)
      std::make_pair((u8)0x05, [this,&ptr]() {
        op_mnem_ = op_dec;
      }),
      //  dec c
      //  dec e
      //  dec l
      //  dec a
      std::make_pair((u8)0x0D, [this,&ptr]() {
        op_mnem_ = op_dec;
      }),

      //  ld b, <imm8>
      //  ld d, <imm8>
      //  ld h, <imm8>
      //  ld (hl), <imm8>
      std::make_pair((u8)0x06, [this,&ptr]() {
        op_mnem_ = op_ld;
      }),
      //  ld c, <imm8>
      //  ld e, <imm8>
      //  ld l, <imm8>
      //  ld a, <imm8>
      std::make_pair((u8)0x0E, [this,&ptr]() {
        op_mnem_ = op_ld;
      }),

      std::make_pair((u8)0x07, [this,&ptr]() { dispatchOnHiNibble(
        //  rlca
        std::make_pair(OpcodeRLCA, [this,&ptr]() {
          op_mnem_ = op_rlca;
        }),

        //  rla
        std::make_pair(OpcodeRLA, [this,&ptr]() {
          op_mnem_ = op_rla;
        }),

        //  daa
        std::make_pair(OpcodeDAA, [this,&ptr]() {
          op_mnem_ = op_daa;
        }),

        //  scf
        std::make_pair(OpcodeSCF, [this,&ptr]() {
          op_mnem_ = op_scf;
        }));
      }),
      std::make_pair((u8)0x0F, [this,&ptr]() { dispatchOnHiNibble(
        //  rrca
        std::make_pair(OpcodeRRCA, [this,&ptr]() {
          op_mnem_ = op_rrca;
        }),

        //  rra
        std::make_pair(OpcodeRRA, [this,&ptr]() {
          op_mnem_ = op_rra;
        }),

        //  cpl 
        std::make_pair(OpcodeCPL, [this,&ptr]() {
          op_mnem_ = op_cpl;
        }),

        //  ccf
        std::make_pair(OpcodeCCF, [this,&ptr]() {
          op_mnem_ = op_ccf;
        }));
      }),

      std::make_pair((u8)0x08, [this,&ptr]() { dispatchOnHiNibble(
          //  ld (<addr16>), SP
          std::make_pair((u8)0x00, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the source address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          //  jr <r8>
          std::make_pair((u8)0x10, [this,&ptr]() {
            op_mnem_ = op_jr;

            // Fetch the jump displacement
            operand_ = *ptr++;
          }),
          //  jr Z, <r8>
          std::make_pair((u8)0x20, [this,&ptr]() {
            op_mnem_ = op_jr;

            // Fetch the jump displacement
            operand_ = *ptr++;
          }),
          //  jr C, <r8>
          std::make_pair((u8)0x30, [this,&ptr]() {
            op_mnem_ = op_jr;

            // Fetch the jump displacement
            operand_ = *ptr++;
          }));
      }),

      //  add hl, bc
      //  add hl, de
      //  add hl, hl
      //  add hl, bc
      std::make_pair((u8)0x09, [this,&ptr]() {
        op_mnem_ = op_add;
      }),

      //  ld a, (bc)
      //  ld a, (de)
      //  ld a, (hl+)
      //  ld a, (hl-)
      std::make_pair((u8)0x0A, [this,&ptr]() {
        op_mnem_ = op_ld;
      })
  );

  return ptr;
}

auto Instruction::disassemble0x80_0xB0(u8 *ptr) -> u8 *
{
  dispatchOnHiNibble(
      std::make_pair((u8)0x80, [this,&ptr]() {
        if(lo_nibble_between(op_, 0x00, 0x07)) {
          op_mnem_ = op_add;
        } else {
          op_mnem_ = op_adc;
        }
      }),

      std::make_pair((u8)0x90, [this,&ptr]() {
        if(lo_nibble_between(op_, 0x00, 0x07)) {
          op_mnem_ = op_sub;
        } else {
          op_mnem_ = op_sbc;
        }
      }),

      std::make_pair((u8)0xA0, [this,&ptr]() {
        if(lo_nibble_between(op_, 0x00, 0x07)) {
          op_mnem_ = op_and;
        } else {
          op_mnem_ = op_xor;
        }
      }),

      std::make_pair((u8)0xB0, [this,&ptr]() {
        if(lo_nibble_between(op_, 0x00, 0x07)) {
          op_mnem_ = op_or;
        } else {
          op_mnem_ = op_cp;
        }
      })
  );

  return ptr;
}

auto Instruction::disassemble0xC0_0xF0(u8 *ptr) -> u8 *
{
  dispatchOnLoNibble(
      std::make_pair((u8)0x00, [this,&ptr]() { dispatchOnHiNibble(
          //  ret nz
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_ret;
          }),
          //  ret nc
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_ret;
          }),

          //  ld (0xFF00+<a8>), a
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the load offset
            operand_ = *ptr++;
          }),
          //  ld a, (0xFF00+<a8>)
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the load offset
            operand_ = *ptr++;
          }));
      }),

      std::make_pair((u8)0x01, [this,&ptr]() {
          op_mnem_ = op_pop;
      }),

      std::make_pair((u8)0x02, [this,&ptr]() { dispatchOnHiNibble(
          //  jp nz, <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_jp;

            // Fetch the jump address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),
          //  jp nc, <a16>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_jp;

            // Fetch the jump address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          //  ld (0xFF00+c), a
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_ld;
          }),
          //  ld a, (0xFF00+c)
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ld;
          }));
      }),

      std::make_pair((u8)0x03, [this,&ptr]() { dispatchOnHiNibble(
          //  jp <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_jp;

            // Fetch the jump address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          std::make_pair((u8)0xD0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xE0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),

          //  di
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_di;
          }));
      }),

      std::make_pair((u8)0x04, [this,&ptr]() { dispatchOnHiNibble(
          //  call nz, <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_call;

            // Fetch the call address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),
          //  call nc, <a16>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_call;

            // Fetch the call address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),


          std::make_pair((u8)0xE0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xF0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }));
      }),

      //  push bc
      //  push de
      //  push hl
      //  push af
      std::make_pair((u8)0x05, [this,&ptr]() {
          op_mnem_ = op_push;
      }),

      std::make_pair((u8)0x06, [this,&ptr]() { dispatchOnHiNibble(
          //  add a, <d8>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_add;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  sub <d8>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_sub;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  and <d8>
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_and;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  or <d8>
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_or;

            // Fetch the immediate data
            operand_ = *ptr++;
          }));
      }),

      //  rst 0x00
      //  rst 0x10
      //  rst 0x20
      //  rst 0x30
      std::make_pair((u8)0x07, [this,&ptr]() {
        op_mnem_ = op_rst;
      }),

      std::make_pair((u8)0x08, [this,&ptr]() { dispatchOnHiNibble(
          //  ret z
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_ret;
          }),
          //  ret c
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_ret;
          }),

          //  add sp, <r8>
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_add;

            // Fetch immediate data
            operand_ = *ptr++;
          }),

          //  ld hl, sp+<r8>
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch immediate data
            operand_ = *ptr++;
          }));
      }),

      std::make_pair((u8)0x09, [this,&ptr]() { dispatchOnHiNibble(
          //  ret
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_ret;
          }),
          //  reti
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_reti;
          }),

          //  jp (hl)
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_jp;
          }),

          //  ld sp, hl
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ld;
          }));
      }),

      std::make_pair((u8)0x0A, [this,&ptr]() { dispatchOnHiNibble(
          //  jp z, <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_jp;

            // Fetch the jump address
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),
          //  jp c, <a16>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_jp;

            // Fetch the jump address
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          //  add (<a16>), a 
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch load address
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),
          //  ld a, (<a16>)
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch load address
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }));
      }),

      std::make_pair((u8)0x0B, [this,&ptr]() { dispatchOnHiNibble(
          //  <cb prefix>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            assert(0 && "decode0xC0_0xF0() called with CB-prefixed instruction!");
          }),

          std::make_pair((u8)0xD0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xE0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),

          //  ei
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_ei;
          }));
      }),

      std::make_pair((u8)0x0C, [this,&ptr]() { dispatchOnHiNibble(
          //  call z, <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_call;

            // Fetch the call address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),
          //  call c, <a16>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_call;

            // Fetch the call address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),


          std::make_pair((u8)0xE0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xF0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }));
      }),

      std::make_pair((u8)0x0D, [this,&ptr]() { dispatchOnHiNibble(
          //  call <a16>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_call;

            // Fetch the call address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          std::make_pair((u8)0xD0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xE0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }),
          std::make_pair((u8)0xF0, [this,&ptr]() {
            throw Disassembler::IllegalOpcodeError(ptr-mem_-1, op_);
          }));
      }),

      std::make_pair((u8)0x0E, [this,&ptr]() { dispatchOnHiNibble(
          //  adc a, <d8>
          std::make_pair((u8)0xC0, [this,&ptr]() {
            op_mnem_ = op_adc;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  sbc <d8>
          std::make_pair((u8)0xD0, [this,&ptr]() {
            op_mnem_ = op_sbc;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  and <d8>
          std::make_pair((u8)0xE0, [this,&ptr]() {
            op_mnem_ = op_xor;

            // Fetch the immediate data
            operand_ = *ptr++;
          }),

          //  or <d8>
          std::make_pair((u8)0xF0, [this,&ptr]() {
            op_mnem_ = op_cp;

            // Fetch the immediate data
            operand_ = *ptr++;
          }));
      }),

      //  rst 0x08
      //  rst 0x18
      //  rst 0x28
      //  rst 0x38
      std::make_pair((u8)0x0F, [this,&ptr]() {
        op_mnem_ = op_rst;
      })
  );

  return ptr;
}

auto Instruction::disassembleCBPrefixed(u8 *ptr) -> u8 *
{
  op_CB_prefixed_ = true;

  // Discard the fetched prefix and fetch the real opcode
  u8 op = *ptr++;
  op_ = op;

  if(hi_nibble_between(op, 0x00, 0x30)) {
    static const OpcodeMnemonic op_mnem[] = {
      op_rlc, op_rrc,
      op_rl, op_rr,
      op_sla, op_sra,
      op_swap, op_srl,
    };

    /*
      0x07 - 0000 0111  // rlc a
      0x0F - 0000 1111  // rrc a
      0x17 - 0001 0111  // rl a
      0x1F - 0001 1111  // rr a
      0x27 - 0010 0111  // sla a
      0x2F - 0010 1111  // sra a
      0x37 - 0011 0111  // swap a
      0x3F - 0011 1111  // srl a
    */
    op_mnem_ = op_mnem[op_.bit(3, 5)];
  } else if(hi_nibble_between(op, 0x40, 0x70)) {    // bit <0-7>, <reg8>
    op_mnem_ = op_bit;
  } else if(hi_nibble_between(op, 0x80, 0xB0)) {    // res <0-7>, <reg8>
    op_mnem_ = op_res;
  } else {                                          // set <0-7>, <reg8>
    op_mnem_ = op_set;
  }

  return ptr;
}

auto Instruction::opcodeToStr() -> std::string
{
  assert(mem_ && op_mnem_ != op_Invalid &&
      "Instruction::toStr() can be called ONLY after decode() on that object!");

  std::string opcode_mnem;
  if(!op_CB_prefixed_) {
    opcode_mnem = op_mnem_to_str(op_mnem_);
  } else {
    opcode_mnem = op_0xCB_mnem_to_str(op_mnem_);
  }

  return opcode_mnem;
}

auto Instruction::operandsToStr() -> std::string
{
  assert(mem_ && op_mnem_ != op_Invalid &&
      "Instruction::operandsToStr() can be called ONLY after Instruction::disassemble()!");

  unsigned num_operands = numOperands();
  if(!num_operands) return "";     // Instruction has no operands

  std::ostringstream os;

  for(auto i = 0; i < num_operands; i++) {
    auto type = operandType(i);

    // Comma between operands
    if(i > 0) os << ", ";

    switch(type) {
    case OperandInvalid: assert(0);   // Unreachable

    case OperandNone:
    case OperandImplied: return "";

    case OperandRSTVector:
      os << util::fmt("%.2xh", RSTVector());
      break;

    case OperandCond:
      os << OperandCondition_to_str(cond());
      break;

    case OperandReg8:
    case OperandReg16:

    case OperandReg16Indirect:
      os << OperandReg_to_str(reg(i));
      break;

    case OperandImm8:
      os << util::fmt("$%.2x", imm8());
      break;

    case OperandImm16:
    case OperandAddress16:
      os << util::fmt("$%.4x", imm16());
      break;

    case OperandRelOffset8:
      os << util::fmt("<$%.4x>", (intptr_t)offset_ + relOffset());
      break;

    case OperandPtr16:
      os << util::fmt("($%.4x)", address());
      break;
    }
  }

  return os.str();
}

Disassembler::IllegalOpcodeError::IllegalOpcodeError(
    ptrdiff_t offset, u8 op
  ) : std::runtime_error(
      util::fmt("illegal opcode 0x%.2x@0x%.4x", (unsigned)op, (unsigned)offset)
  )
{ }

auto Disassembler::IllegalOperandError::format_error(
    ptrdiff_t offset, u8 op, OperandSize op_size, unsigned operand
  ) -> std::string
{
#define FMT_BASE "illegal operand for opcode 0x%.2x@0x%.4x (%s) -> "
#define FMT_ARGS (unsigned)op, (unsigned)offset,                                    \
                  Instruction::op_mnem_to_str((OpcodeMnemonic)op).data(), operand

  switch(op_size) {
  case OperandSize::Operand_u8:  return util::fmt(FMT_BASE "0x%.2x", FMT_ARGS);
  case OperandSize::Operand_u16: return util::fmt(FMT_BASE "0x%.4x", FMT_ARGS);
  }

  assert(0);    // Unreachable
  return "";

#undef FMT_BASE
#undef FMT_ARGS
}

Disassembler::IllegalOperandError::IllegalOperandError(
    ptrdiff_t offset, u8 op, OperandSize op_size, unsigned operand
  ) : std::runtime_error(format_error(offset, op, op_size, operand))
{ }
    

auto Disassembler::begin(u8 *mem) -> Disassembler&
{
  // Setup the memory pointer and the cursor
  //   to it's beginning
  mem_ = cursor_ = mem;

  return *this;
}

auto Disassembler::singleStep() -> std::string
{
  return "";
}

}
