#pragma once

#include <types.h>
#include <util/natural.h>
#include <util/bit.h>

#include <type_traits>
#include <limits>
#include <string>
#include <array>
#include <tuple>
#include <variant>
#include <optional>

#include <cstddef>

namespace brgb::huc6280disasm {

enum OpcodeMnemonic : u8 {
  op_Invalid,

  op_nop,
  op_brk,
  op_csl, op_csh,
  op_lda, op_ldx, op_ldy,
  op_sta, op_stx, op_sty, op_stz,
  op_cla, op_clx, op_cly,
  op_tax, op_txa, op_tay, op_tya, op_tsx, op_txs,
  op_sax, op_say, op_sxy,
  op_pha, op_phx, op_phy, op_php, op_pla, op_plx, op_ply, op_plp,
  op_tam, op_tma,
  op_sec, op_clc, op_sed, op_cld, op_sei, op_cli, op_clv, op_set,
  op_adc, op_sbc, op_and, op_ora, op_eor, op_asl, op_lsr, op_ror, op_rol,
  op_inc, op_inx, op_iny, op_dec, op_dex, op_dey,
  op_smbi, op_rmbi,
  op_trb, op_tsb, op_tst,
  op_cmp, op_cpx, op_cpy,
  op_bit,
  op_jmp, op_jsr, op_bsr,
  op_rts, op_rti,
  op_bra, op_bbsi, op_bbri, op_bcc, op_bcs, op_beq, op_bne, op_bpl, op_bmi, op_bvc, op_bvs,
  op_st0, op_st1, op_st2,
  op_tii, op_tdd, op_tia, op_tai, op_tin,
};

class Instruction {
public:
  enum AddressingMode {
    Implied,
    Immediate8, Immediate16,
    ZeroPage, ZeroPage_X, ZeroPage_Y,
    Absolute, Absolute_X, Absolute_Y,
    IndexedIndirect8_X, Indirect8_Y,
    Indirect16, IndexedIndirect16_X,
    PCRelative,   // Used as offset in branch instructions
  };

  static auto OpcodeMnemonic_to_str(OpcodeMnemonic op) -> std::string;

  // 'mem' is a pointer to the base of the binary being diassembled
  Instruction(u8 *mem);

  // Populates the Instruction object with data at 'ptr'
  //   and returns 'ptr' advanced appropriately i.e. by
  //   the width of the opcode and it's operands (if any)
  auto disassemble(u8 *ptr) -> u8*;

  auto toStr() -> std::string;

private:
  using Operand16 = Natural<16>;
  using Operand8  = Natural<8>;

  using Operand = std::tuple<AddressingMode, std::variant<Operand8, Operand16>>;

  template <typename OperandSize>
  auto appendOperand(AddressingMode addr_mode, OperandSize operand) -> Operand&
  {
    static_assert(std::is_same_v<OperandSize, Operand8> || std::is_same_v<OperandSize, Operand16>,
        "The type argument must be either Operand8 or Operand16!");
    
    // Grab a reference to the Operand and increment the number
    //   of operands 'num_operands_' for this Instruction
    auto& opr = operands_.at(num_operands_++);
    opr.emplace(addr_mode, OperandSize(operand));

    return opr.value();
  }

  auto appendOperandByVariant(
      AddressingMode addr_mode, std::variant<Operand8, Operand16> operand
    ) -> Operand&
  {
    auto& opr = operands_.at(num_operands_++);  // Grab a reference and increment the number
                                                //   of the Instruction's operands
    opr.emplace(addr_mode, operand);

    return opr.value();
  }

  // Returns 'true' when OpcodeMnemonic_to_str() returns a mnemonic
  //   which contains printf-style formatting %<format> sequences
  //   and thus - needs further processing
  auto mnemonicNeedsFmt() -> bool;

  // Base address of the binary being diassembled
  u8 *mem_ = nullptr;
  // Offset of this instruction in the binary (i.e. relative to 'mem_')
  uintptr_t offset_ = std::numeric_limits<uintptr_t>::max();

  OpcodeMnemonic op_mnem_ = op_Invalid;
  u8 op_;

  size_t num_operands_ = 0;
  std::array<std::optional<Operand>, 3> operands_ = { std::nullopt, std::nullopt, std::nullopt };
};

class Disassembler {
public:
  // Begin diassemble of a binary at the given address
  //   - Resets the internal cursor to to 'mem' (the beginning of the binary)
  auto begin(u8 *mem) -> Disassembler&;

  // Disassemble a single instruction and advance the internal cursor
  auto singleStep() -> std::string;

private:
  u8 *mem_ = nullptr;
  u8 *cursor_ = nullptr;
};

}
