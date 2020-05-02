#include <device/huc6280/disassembler.h>

#include <util/natural.h>
#include <util/bit.h>
#include <util/format.h>

#include <sstream>
#include <unordered_map>

#include <cassert>

namespace brgb::huc6280disasm {

inline auto lo_nibble(u8 x) -> u8 { return x & 0x0F; }
inline auto hi_nibble(u8 x) -> u8 { return x & 0xF0; }

inline auto lo_nibble_between(u8 x, u8 lo, u8 hi) -> bool
{
  return lo_nibble(x) >= lo && hi_nibble(x) <= hi;
}

inline auto hi_nibble_between(u8 x, u8 lo, u8 hi) -> bool
{
  return hi_nibble(x) >= lo && hi_nibble(x) <= hi;
}

static const std::unordered_map<OpcodeMnemonic, std::string> p_op_mnem_to_str = {
  { op_nop, "nop" },
  { op_brk, "brk" },
  { op_csl, "csl" }, { op_csh, "csh" },
  { op_lda, "lda" }, { op_ldx, "ldx" }, { op_ldy, "ldy" },
  { op_sta, "sta" }, { op_stx, "stx" }, { op_sty, "sty" }, { op_stz, "stz" },
  { op_cla, "cla" }, { op_clx, "clx" }, { op_cly, "cly" },
  { op_tax, "tax" }, { op_txa, "txa" }, { op_tay, "tay" }, { op_tya, "tya" },
  { op_tsx, "tsx" }, { op_txs, "txs" },
  { op_sax, "sax" }, { op_say, "say" }, { op_sxy, "sxy" },
  { op_pha, "pha" }, { op_phx, "phx" }, { op_phy, "phy" }, { op_php, "php" },
  { op_pla, "pla" }, { op_plx, "plx" }, { op_ply, "ply" }, { op_plp, "plp" },
  { op_tam, "tam" }, { op_tma, "tma" },
  { op_sec, "sec" }, { op_clc, "clc" }, { op_sed, "sed" }, { op_cld, "cld" }, 
  { op_sei, "sei" }, { op_cli, "cli" }, { op_clv, "clv" }, { op_set, "set" },
  { op_adc, "adc" }, { op_sbc, "sbc" }, { op_and, "and" }, { op_ora, "ora" },
  { op_eor, "eor" }, { op_asl, "asl" }, { op_lsr, "lsr" }, { op_ror, "ror" }, { op_rol, "rol" },
  { op_smbi, "smb%u" }, { op_rmbi, "rmb%u" },
  { op_trb, "trb" }, { op_tsb, "tsb" }, { op_tst, "tst" },
  { op_cmp, "cmp" }, { op_cpx, "cpx" }, { op_cpy, "cpy" },
  { op_bit, "bit" },
  { op_jmp, "jmp" }, { op_jsr, "jsr" },
  { op_rts, "rts" }, { op_rti, "rti" },
  { op_bra, "bra" }, { op_bcc, "bcc" }, { op_bcs, "bcs" }, { op_beq, "beq" }, { op_bne, "bne" },
  { op_bpl, "bpl" }, { op_bmi, "bmi" }, { op_bvc, "bvc" }, { op_bvs, "bvs" },
  { op_bbsi, "bbs%u" }, { op_bbri, "bbr%u" },
  { op_inc, "inc" }, { op_inx, "inx" }, { op_iny, "iny" },
  { op_dec, "dec" }, { op_dex, "dex" }, { op_dey, "dey" },
  { op_st0, "st0" }, { op_st1, "st1" }, { op_st2, "st2" },
  { op_tii, "tii" }, { op_tdd, "tdd" }, { op_tia, "tia" }, { op_tai, "tai" }, { op_tin, "tin" },
};
auto Instruction::OpcodeMnemonic_to_str(OpcodeMnemonic op) -> std::string
{
  auto it = p_op_mnem_to_str.find(op);

  return it != p_op_mnem_to_str.end() ? it->second : "<invalid>";
}

Instruction::Instruction(u8 *mem) :
  mem_(mem)
{
}

auto Instruction::disassemble(u8 *ptr) -> u8*
{
  offset_ = ptr - mem_;   // Calculate this instruction's offset in the binary
  op_ = *ptr++;       // ... and fetch the opcode

  Natural<8> op = op_;

  auto fetch_operand_by_addressing_mode = [this,&ptr](
      AddressingMode addr_mode
    ) -> std::variant<Operand8, Operand16>
  {
    switch(addr_mode) {
    case Immediate8: return Operand8(*ptr++);

    case Indirect8:
    case ZeroPage:
    case ZeroPage_X:
    case ZeroPage_Y: return Operand8(*ptr++);

    case Immediate16:
    case Absolute:
    case Absolute_X:
    case Absolute_Y:
    case IndexedIndirect16_X: {
      Operand16 operand;

      operand.bit(0, 7)  = *ptr++;
      operand.bit(8, 15) = *ptr++;

      return operand;
    }

    case IndexedIndirect8_X:
    case Indirect8_Y: return Operand8(*ptr++);

    case PCRelative: return Operand8(*ptr++);

    default: assert(0 && "The given AddressingMode does not have a related Operand!");
    }

    return { Operand8(0) };   // Unreachable
  };

  auto with_operand = [this,&ptr,&fetch_operand_by_addressing_mode](
      OpcodeMnemonic mnem, AddressingMode addr_mode
  ) {
    op_mnem_ = mnem;

    auto operand = fetch_operand_by_addressing_mode(addr_mode);

    appendOperandByVariant(addr_mode, operand);
  };

  auto branch_instruction = [this,&ptr](OpcodeMnemonic mnem) {
    op_mnem_ = mnem;
    appendOperand<Operand8>(PCRelative, *ptr++);
  };

  auto tst_instruction = [this,&ptr,&fetch_operand_by_addressing_mode](
      AddressingMode addr_mode
  ) {
    op_mnem_ = op_tst;

    auto imm  = fetch_operand_by_addressing_mode(Immediate8);
    auto addr = fetch_operand_by_addressing_mode(addr_mode);

    appendOperandByVariant(Immediate8, imm);
    appendOperandByVariant(addr_mode, addr);
  };

  auto block_transfer_instruction = [this,&ptr,&fetch_operand_by_addressing_mode](
      OpcodeMnemonic mnem
  ) {
    op_mnem_ = mnem;

    auto src = fetch_operand_by_addressing_mode(Immediate16);
    auto dst = fetch_operand_by_addressing_mode(Immediate16);
    auto len = fetch_operand_by_addressing_mode(Immediate16);

    appendOperandByVariant(Immediate16, src);
    appendOperandByVariant(Immediate16, dst);
    appendOperandByVariant(Immediate16, len);
  };

  switch(op.get()) {
  case 0xEA: op_mnem_ = op_nop; break;
  case 0x00: op_mnem_ = op_brk; break;

  case 0x54: op_mnem_ = op_csl;
  case 0xD4: op_mnem_ = op_csh;

  case 0xA9: with_operand(op_lda, Immediate8); break;
  case 0xA5: with_operand(op_lda, ZeroPage); break;
  case 0xB5: with_operand(op_lda, ZeroPage_X); break;
  case 0xAD: with_operand(op_lda, Absolute); break;
  case 0xBD: with_operand(op_lda, Absolute_X); break;
  case 0xB9: with_operand(op_lda, Absolute_Y); break;
  case 0xB2: with_operand(op_lda, Indirect8); break;
  case 0xA1: with_operand(op_lda, IndexedIndirect8_X); break;
  case 0xB1: with_operand(op_lda, Indirect8_Y); break;

  case 0xA2: with_operand(op_ldx, Immediate8); break;
  case 0xA6: with_operand(op_ldx, ZeroPage); break;
  case 0xB6: with_operand(op_ldx, ZeroPage_Y); break;
  case 0xAE: with_operand(op_ldx, Absolute); break;
  case 0xBE: with_operand(op_ldx, Absolute_Y); break;

  case 0xA0: with_operand(op_ldy, Immediate8); break;
  case 0xA4: with_operand(op_ldy, ZeroPage); break;
  case 0xB4: with_operand(op_ldy, ZeroPage_X); break;
  case 0xAC: with_operand(op_ldy, Absolute); break;
  case 0xBC: with_operand(op_ldy, Absolute_X); break;

  case 0x85: with_operand(op_sta, ZeroPage); break;
  case 0x95: with_operand(op_sta, ZeroPage_X); break;
  case 0x8D: with_operand(op_sta, Absolute); break;
  case 0x9D: with_operand(op_sta, Absolute_X); break;
  case 0x99: with_operand(op_sta, Absolute_Y); break;
  case 0x92: with_operand(op_sta, Indirect8); break;
  case 0x81: with_operand(op_sta, IndexedIndirect8_X); break;
  case 0x91: with_operand(op_sta, Indirect8_Y); break;

  case 0x86: with_operand(op_stx, ZeroPage); break;
  case 0x96: with_operand(op_stx, ZeroPage_Y); break;
  case 0x8E: with_operand(op_stx, Absolute); break;

  case 0x84: with_operand(op_sty, ZeroPage); break;
  case 0x94: with_operand(op_sty, ZeroPage_X); break;
  case 0x8C: with_operand(op_sty, Absolute); break;

  case 0x64: with_operand(op_stz, ZeroPage); break;
  case 0x74: with_operand(op_stz, ZeroPage_X); break;
  case 0x9C: with_operand(op_stz, Absolute); break;
  case 0x9E: with_operand(op_stz, Absolute_X); break;

  case 0x62: op_mnem_ = op_cla; break;
  case 0x82: op_mnem_ = op_clx; break;
  case 0xC2: op_mnem_ = op_cly; break;

  case 0xAA: op_mnem_ = op_tax; break;
  case 0x8A: op_mnem_ = op_txa; break;
  case 0xA8: op_mnem_ = op_tay; break;
  case 0x98: op_mnem_ = op_tya; break;
  case 0xBA: op_mnem_ = op_tsx; break;
  case 0x9A: op_mnem_ = op_txs; break;

  case 0x22: op_mnem_ = op_sax; break;
  case 0x42: op_mnem_ = op_say; break;
  case 0x02: op_mnem_ = op_sxy; break;

  case 0x48: op_mnem_ = op_pha; break;
  case 0x08: op_mnem_ = op_php; break;
  case 0xDA: op_mnem_ = op_phx; break;
  case 0x5A: op_mnem_ = op_phy; break;
  case 0x68: op_mnem_ = op_pla; break;
  case 0x28: op_mnem_ = op_plp; break;
  case 0xFA: op_mnem_ = op_plx; break;
  case 0x7A: op_mnem_ = op_ply; break;

  case 0x53: with_operand(op_tam, Immediate8); break;
  case 0x43: with_operand(op_tma, Immediate8); break;

  case 0x38: op_mnem_ = op_sec; break;
  case 0x18: op_mnem_ = op_clc; break;
  case 0xF8: op_mnem_ = op_sed; break;
  case 0xD8: op_mnem_ = op_cld; break;
  case 0x78: op_mnem_ = op_sei; break;
  case 0x58: op_mnem_ = op_cli; break;
  case 0xB8: op_mnem_ = op_clv; break;
  case 0xF4: op_mnem_ = op_set; break;

  case 0x69: with_operand(op_adc, Immediate8); break;
  case 0x65: with_operand(op_adc, ZeroPage); break;
  case 0x75: with_operand(op_adc, ZeroPage_X); break;
  case 0x6D: with_operand(op_adc, Absolute); break;
  case 0x7D: with_operand(op_adc, Absolute_X); break;
  case 0x79: with_operand(op_adc, Absolute_Y); break;
  case 0x72: with_operand(op_adc, Indirect8); break;
  case 0x61: with_operand(op_adc, IndexedIndirect8_X); break;
  case 0x71: with_operand(op_adc, Indirect8_Y); break;

  case 0xE9: with_operand(op_sbc, Immediate8); break;
  case 0xE5: with_operand(op_sbc, ZeroPage); break;
  case 0xF5: with_operand(op_sbc, ZeroPage_X); break;
  case 0xED: with_operand(op_sbc, Absolute); break;
  case 0xFD: with_operand(op_sbc, Absolute_X); break;
  case 0xF9: with_operand(op_sbc, Absolute_Y); break;
  case 0xF2: with_operand(op_sbc, Indirect8); break;
  case 0xE1: with_operand(op_sbc, IndexedIndirect8_X); break;
  case 0xF1: with_operand(op_sbc, Indirect8_Y); break;

  case 0x29: with_operand(op_and, Immediate8); break;
  case 0x25: with_operand(op_and, ZeroPage); break;
  case 0x35: with_operand(op_and, ZeroPage_X); break;
  case 0x2D: with_operand(op_and, Absolute); break;
  case 0x3D: with_operand(op_and, Absolute_X); break;
  case 0x39: with_operand(op_and, Absolute_Y); break;
  case 0x32: with_operand(op_and, Indirect8); break;
  case 0x21: with_operand(op_and, IndexedIndirect8_X); break;
  case 0x31: with_operand(op_and, Indirect8_Y); break;

  case 0x09: with_operand(op_ora, Immediate8); break;
  case 0x05: with_operand(op_ora, ZeroPage); break;
  case 0x15: with_operand(op_ora, ZeroPage_X); break;
  case 0x0D: with_operand(op_ora, Absolute); break;
  case 0x1D: with_operand(op_ora, Absolute_X); break;
  case 0x19: with_operand(op_ora, Absolute_Y); break;
  case 0x12: with_operand(op_ora, Indirect8); break;
  case 0x01: with_operand(op_ora, IndexedIndirect8_X); break;
  case 0x11: with_operand(op_ora, Indirect8_Y); break;

  case 0x49: with_operand(op_eor, Immediate8); break;
  case 0x45: with_operand(op_eor, ZeroPage); break;
  case 0x55: with_operand(op_eor, ZeroPage_X); break;
  case 0x4D: with_operand(op_eor, Absolute); break;
  case 0x5D: with_operand(op_eor, Absolute_X); break;
  case 0x59: with_operand(op_eor, Absolute_Y); break;
  case 0x52: with_operand(op_eor, Indirect8); break;
  case 0x41: with_operand(op_eor, IndexedIndirect8_X); break;
  case 0x51: with_operand(op_eor, Indirect8_Y); break;

  case 0x0A: op_mnem_ = op_asl; break;
  case 0x06: with_operand(op_asl, ZeroPage); break;
  case 0x16: with_operand(op_asl, ZeroPage_X); break;
  case 0x0E: with_operand(op_asl, Absolute); break;
  case 0x1E: with_operand(op_asl, Absolute_X); break;

  case 0x4A: op_mnem_ = op_lsr; break;
  case 0x46: with_operand(op_lsr, ZeroPage); break;
  case 0x56: with_operand(op_lsr, ZeroPage_X); break;
  case 0x4E: with_operand(op_lsr, Absolute); break;
  case 0x5E: with_operand(op_lsr, Absolute_X); break;

  case 0x6A: op_mnem_ = op_ror; break;
  case 0x66: with_operand(op_ror, ZeroPage); break;
  case 0x76: with_operand(op_ror, ZeroPage_X); break;
  case 0x6E: with_operand(op_ror, Absolute); break;
  case 0x7E: with_operand(op_ror, Absolute_X); break;

  case 0x2A: op_mnem_ = op_rol; break;
  case 0x26: with_operand(op_rol, ZeroPage); break;
  case 0x36: with_operand(op_rol, ZeroPage_X); break;
  case 0x2E: with_operand(op_rol, Absolute); break;
  case 0x3E: with_operand(op_rol, Absolute_X); break;

  case 0x1A: op_mnem_ = op_inc; break;
  case 0xE6: with_operand(op_inc, ZeroPage); break;
  case 0xF6: with_operand(op_inc, ZeroPage_X); break;
  case 0xEE: with_operand(op_inc, Absolute); break;
  case 0xFE: with_operand(op_inc, Absolute_X); break;

  case 0xE8: op_mnem_ = op_inx; break;
  case 0xC8: op_mnem_ = op_iny; break;

  case 0xC6: with_operand(op_dec, ZeroPage); break;
  case 0xD6: with_operand(op_dec, ZeroPage_X); break;
  case 0xCE: with_operand(op_dec, Absolute); break;
  case 0xDE: with_operand(op_dec, Absolute_X); break;

  case 0xCA: op_mnem_ = op_dex; break;
  case 0x88: op_mnem_ = op_dey; break;

  case 0x14: with_operand(op_trb, ZeroPage); break;
  case 0x1C: with_operand(op_trb, Absolute); break;

  case 0x04: with_operand(op_tsb, ZeroPage); break;
  case 0x0C: with_operand(op_tsb, Absolute); break;

  case 0x83: tst_instruction(ZeroPage); break;
  case 0xA3: tst_instruction(ZeroPage_X); break;
  case 0x93: tst_instruction(Absolute); break;
  case 0xB3: tst_instruction(Absolute_X); break;

  case 0xC9: with_operand(op_cmp, Immediate8); break;
  case 0xC5: with_operand(op_cmp, ZeroPage); break;
  case 0xD5: with_operand(op_cmp, ZeroPage_X); break;
  case 0xCD: with_operand(op_cmp, Absolute); break;
  case 0xDD: with_operand(op_cmp, Absolute_X); break;
  case 0xD9: with_operand(op_cmp, Absolute_Y); break;
  case 0xD2: with_operand(op_cmp, Indirect8); break;
  case 0xC1: with_operand(op_cmp, IndexedIndirect8_X); break;
  case 0xD1: with_operand(op_cmp, Indirect8_Y); break;

  case 0xE0: with_operand(op_cpx, Immediate8); break;
  case 0xE4: with_operand(op_cpx, ZeroPage); break;
  case 0xEC: with_operand(op_cpx, Absolute); break;

  case 0xC0: with_operand(op_cpy, Immediate8); break;
  case 0xC4: with_operand(op_cpy, ZeroPage); break;
  case 0xCC: with_operand(op_cpy, Absolute); break;

  case 0x89: with_operand(op_bit, Immediate8); break;
  case 0x24: with_operand(op_bit, ZeroPage); break;
  case 0x34: with_operand(op_bit, ZeroPage_X); break;
  case 0x2C: with_operand(op_bit, Absolute); break;
  case 0x3C: with_operand(op_bit, Absolute_X); break;

  case 0x4C: with_operand(op_jmp, Absolute); break;
  case 0x6C: with_operand(op_jmp, Indirect8); break;
  case 0x7C: with_operand(op_jmp, IndexedIndirect16_X); break;

  case 0x20: with_operand(op_jsr, Absolute); break;

  case 0x44: with_operand(op_bsr, PCRelative); break;

  case 0x60: op_mnem_ = op_rts; break;
  case 0x40: op_mnem_ = op_rti; break;

  case 0x10: branch_instruction(op_bpl); break;
  case 0x30: branch_instruction(op_bmi); break;
  case 0x50: branch_instruction(op_bvc); break;
  case 0x70: branch_instruction(op_bvs); break;
  case 0x80: branch_instruction(op_bra); break;
  case 0x90: branch_instruction(op_bcc); break;
  case 0xB0: branch_instruction(op_bcs); break;
  case 0xD0: branch_instruction(op_bne); break;
  case 0xF0: branch_instruction(op_beq); break;

  case 0x03: with_operand(op_st0, Immediate8); break;
  case 0x13: with_operand(op_st1, Immediate8); break;
  case 0x23: with_operand(op_st2, Immediate8); break;

  case 0x73: block_transfer_instruction(op_tii); break;
  case 0xC3: block_transfer_instruction(op_tdd); break;
  case 0xD3: block_transfer_instruction(op_tin); break;
  case 0xE3: block_transfer_instruction(op_tia); break;
  case 0xF3: block_transfer_instruction(op_tai); break;
  }

  // If the above switch statement resolved the opcode
  //   and it's operands - return from the method
  if(op_mnem_ != op_Invalid) return ptr;

  //  RMBi $zp
  //  SMBi $zp
  if(lo_nibble(op) == 0x07) {
    if(hi_nibble_between(op, 0x00, 0x70)) {
      op_mnem_ = op_rmbi;
    } else /* 0x80 >= lo_nibble <= 0xF0 */ {
      op_mnem_ = op_smbi;
    }

    u8 zp = *ptr++;

    appendOperand<Operand8>(ZeroPage, zp);

    return ptr;
  }

  //  BBRi $zp, $rel
  //  BBSi $zp, $rel
  if(lo_nibble(op) == 0x0F) {
    if(hi_nibble_between(op, 0x00, 0x70)) {
      op_mnem_ = op_bbri;
    } else /* 0x80 >= lo_nibble <= 0xF0 */ {
      op_mnem_ = op_bbsi;
    }

    u8 zp  = *ptr++;
    u8 rel = *ptr++;

    appendOperand<Operand8>(ZeroPage, zp);
    appendOperand<Operand16>(PCRelative, rel);

    return ptr;
  }

  return ptr;
}

auto Instruction::toStr() -> std::string
{
  assert(op_mnem_ != op_Invalid && "Instruction::toStr(): disassemble MUST be called before!");

  std::ostringstream out;

  auto mnem = OpcodeMnemonic_to_str(op_mnem_);
  if(!mnemonicNeedsFmt(op_mnem_)) {
    out << mnem;
  } else {
    // For RMBi, SMBi, BBRi, BBSi instructions the hi nibble
    //   contains the 'i' index in the lowest 3 bits
    out << fmtMnemonic(mnem, op_.bit(4, 7) & 0b0000'0111);
  }

  // Pad the output (just the mnemonic for now) to 4 columns
  while(out.tellp() < 4) out << ' ';

  for(size_t i = 0; i < num_operands_; i++) {
    auto& operand = operands_.at(i);
    assert(operand.has_value());    // Sanity check

    auto [addr_mode, val] = operand.value();

    auto val_u8 = [&]() -> unsigned { return std::get<Operand8>(val).get(); };
    auto val_i8 = [&]() -> int      { return (i8)std::get<Operand8>(val).get(); };

    auto val_u16 = [&]() -> unsigned { return std::get<Operand16>(val).get(); };

    if(i == 0 /* this is the first operand */) {
      out << ' ';
    } else /* further operands are separated by commas */ {
      out << ", ";
    }

    switch(addr_mode) {
    case Implied: break;

    case Immediate8:  out << util::fmt("#$%.2x", val_u8()); break;
    case Immediate16: out << util::fmt("$%.4x", val_u16()); break;

    case Indirect8:  out << util::fmt("($%.2x)", val_u8()); break;
    case ZeroPage:   out << util::fmt("$%.2x", val_u8()); break;
    case ZeroPage_X: out << util::fmt("$%.2x, X", val_u8()); break;
    case ZeroPage_Y: out << util::fmt("$%.2x, Y", val_u8()); break;

    case Absolute:   out << util::fmt("$%.4x", val_u16()); break;
    case Absolute_X: out << util::fmt("$%.4x, X", val_u16()); break;
    case Absolute_Y: out << util::fmt("$%.4x, Y", val_u16()); break;

    case IndexedIndirect8_X: out << util::fmt("($%.2x, X)", val_u8()); break;
    case Indirect8_Y:        out << util::fmt("($%.2x), Y", val_u8()); break;

    case IndexedIndirect16_X: out << util::fmt("($%.4x, X)", val_u16()); break;

    case PCRelative: out << util::fmt("<$%.4x>", offset_ + val_i8()+2); break;
    }
  }

  return out.str();
}

auto Instruction::mnemonicNeedsFmt(OpcodeMnemonic mnem) -> bool
{
  assert(mnem != op_Invalid);

  switch(mnem) {
  case op_rmbi:
  case op_smbi:

  case op_bbri:
  case op_bbsi: return true;
  }

  return false;
}

auto Instruction::fmtMnemonic(std::string fmt, unsigned i) -> std::string
{
  assert(fmt.find("%u") != std::string::npos &&
      "The given mnemonic format string doesn't actually contain any format specifiers!");

  return util::fmt(fmt.data(), i);
}

auto Disassembler::begin(u8 *mem) -> Disassembler&
{
  // Setup the binary's address and the current instruction cursor
  mem_ = cursor_ = mem;

  return *this;
}

auto Disassembler::singleStep() -> std::string
{
  Instruction instruction(mem_);
  u8 *current_instruction = cursor_;

  std::ostringstream out;

  // Append the instruction's offset on the left
  out << util::fmt("%.4X      ", cursor_ - mem_);

  // Disassemble and append the instruction itself
  cursor_ = instruction.disassemble(cursor_);
  out << instruction.toStr();

  // Pad the output to 30 columns (or add a single space
  //   in case it's width exceedes 30)
  do { out << ' '; } while(out.tellp() < 30);

  // Write the raw bytes that make up the instruction on the right
  out << ';';
  while(current_instruction < cursor_) {
    out << util::fmt(" %.2X", *current_instruction);

    current_instruction++;
  }
  out << '\n';

  return out.str();
}

}
