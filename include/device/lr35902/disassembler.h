#pragma once

#include <array>
#include <types.h>
#include <util/format.h>
#include <util/natural.h>
#include <util/bit.h>

#include <limits>
#include <utility>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <array>
#include <tuple>

#include <cstddef>

namespace brgb::lr35902 {

enum OpcodeMnemonic : u8 {
  op_Invalid,

  op_nop,
  op_stop, op_halt,
  op_jp, op_jr,
  op_ld, op_ldh,
  op_inc, op_dec,
  op_rlca, op_rla, op_rrca, op_rra,
  op_daa,
  op_cpl,
  op_scf, op_ccf,
  op_add, op_adc, op_sub, op_sbc,
  op_and, op_or, op_xor,
  op_cp,
  op_call,
  op_ret, op_reti,
  op_push, op_pop,
  op_ei, op_di,
  op_rst,

  // CB-prefixed opcodes
  op_rlc, op_rl, op_rrc, op_rr,
  op_sla, op_sra, op_srl,
  op_swap,
  op_bit, op_res, op_set,
};

using Opcode = Natural<8>;

class Instruction {
public:
  enum OperandType {
    OperandInvalid,

    OperandNone,
    OperandImplied,   // Ex. the 'A' register in RLCA,
                      //     carry flag in SCF/CCF,
                      //     bit index in BIT,RES,SET CB-prefix opcodes
    OperandRSTVector,
    OperandCond,
    OperandReg8, OperandReg16,
    OperandImm8, OperandImm16,
    OperandRelOffset8,
    OperandAddress16,
    OperandReg16Indirect, OperandPtr16,

    // Used only with ldh
    OperandLDHOffset8, OperandLDHRegC,

    // Used with 0xCB-prefixed instructions
    OperandBitIndex,
  };

  enum OperandReg {
    RegInvalid,

    RegA, RegF,
    RegB, RegC,
    RegD, RegE,
    RegH, RegL,

    RegAF,
    RegBC,
    RegDE,
    RegHL,

    RegSP,

    // OperandReg16Indirect
    RegBCInd, RegDEInd, RegHLInd,    // (bc), (de), (hl)
    RegHLIInd,   // (hl+)
    RegHLDInd,   // (hl-)
  };

  enum OperandCondition {
    ConditionInvalid,
    ConditionC, ConditionNC,
    ConditionZ, ConditionNZ,
  };

  enum : u8 {
    RSTVectorInvalid = 0xFF,
  };

  static constexpr u8 CB_prefix = 0xCB;

  static auto OperandReg_to_str(OperandReg reg) -> std::string;
  static auto OperandCondition_to_str(OperandCondition cond) -> std::string;

  // Returns an opcode's mnemonic
  static auto op_mnem_to_str(OpcodeMnemonic op) -> std::string;
  // Returns the mnemonic for a CB-prefixed opcode
  static auto op_0xCB_mnem_to_str(OpcodeMnemonic op) -> std::string;

  // 'mem' is a pointer to the base of the binary being diassembled
  Instruction(u8 *mem);

  // Populates the Instruction object with data at 'ptr'
  //   and returns 'ptr' advanced appropriately i.e. by
  //   the width of the opcode and it's operands (if any)
  auto disassemble(u8 *ptr) -> u8 *;

  // Returns the number of operands for the Instruction's opcode
  auto numOperands() -> unsigned;

  auto operandType(unsigned which = 0) -> OperandType;

  auto reg(unsigned which = 0) -> OperandReg;
  auto imm8() -> u8;
  auto imm16() -> u16;
  auto address() -> u16;
  auto relOffset() -> i8;
  auto cond() -> OperandCondition;
  auto RSTVector() -> u8;

  // For 0xCB-prefixed instructions
  auto bitIndex() -> unsigned;

  auto toStr() -> std::string;

private:
  auto disassemble0x00_0x30(u8 *ptr) -> u8 *;
  auto disassemble0x80_0xB0(u8 *ptr) -> u8 *;
  auto disassemble0xC0_0xF0(u8 *ptr) -> u8 *;
  auto disassembleCBPrefixed(u8 *ptr) -> u8 *;

  auto opcodeToStr() -> std::string;

  auto operandsToStr() -> std::string;

  // 'op_' MUST have been assigned before calling this method!
  //
  //   Usage:
  //      dispatchOnLoNibble(
  //          std::make_pair(0x0, []() { puts("lo nibble is 0x0!"); }),
  //          std::make_pair(0x2, []() { puts("lo nibble is 0x2!"); }),
  //          std::make_pair(0x8, []() { puts("lo nibble is 0x8!"); })
  //      );
  template <typename... Fn>
  auto dispatchOnLoNibble(std::pair<u8 /* lo_nibble */, Fn>... handlers) -> void
  {
    u8 op = op_;

    // This expression matches op against each pair in 'handlers'
    //   and by short-circuiting calls the pair's Fn (handlers.second)
    //
    //  - The handler's Fn return type is void which would disallow
    //    the usage of &&, so the comma operator is used to coerce
    //    it into int (the 0 in the marked expession below)
    //                                  vvvvvvvvvvvvvvvvvvvv
    ( ((op & 0x0F) == handlers.first && (handlers.second(), 0)), ... );
  }

  template <typename... Fn>
  auto dispatchOnHiNibble(std::pair<u8 /* hi_nibble */, Fn>... handlers) -> void
  {
    u8 op = op_;

    ( ((op & 0xF0) == handlers.first && (handlers.second(), 0)), ... );
  }

  // Base address of the binary being diassembled
  u8 *mem_ = nullptr;
  // Offset of this instruction in the binary
  uintptr_t offset_ = std::numeric_limits<uintptr_t>::max();

  bool op_CB_prefixed_ = false;
  Opcode op_;
  OpcodeMnemonic op_mnem_ = op_Invalid;

  Natural<16> operand_ = 0;
  BitRange<16, 0, 7> operand_lo_{ operand_.ptr() };
  BitRange<16, 8, 15> operand_hi_{ operand_.ptr() };
};

// TODO:
//   - Output non-instruction bytes in their raw form (using
//     a 'db' directive) instead of throwing an exception
//   - Scan the instruction stream in begin() for branch
//     targets and output labels appropriately during
//     actual disassembly
class Disassembler {
public:
  struct IllegalOpcodeError final : public std::runtime_error {
    IllegalOpcodeError(ptrdiff_t offset, u8 op);
  };

  struct IllegalOperandError final : public std::runtime_error {
    static auto from_u8_operand(ptrdiff_t offset, u8 op, u8 operand) -> IllegalOperandError;
    static auto from_u16_operand(ptrdiff_t offset, u8 op, u16 operand) -> IllegalOperandError;

  private:
    enum class OperandSize {
      Operand_u8, Operand_u16,
    };

    auto static format_error(
        ptrdiff_t offset, u8 op, OperandSize op_size, unsigned operand
      ) -> std::string;

    IllegalOperandError(ptrdiff_t offset, u8 op, OperandSize op_size, unsigned operand);
  };

  // Begin disassembling of a binary at the given address
  //   - Resets the internal cursor to 'mem' (i.e. the beginning of the binary)
  auto begin(u8 *mem) -> Disassembler&;

  // Disassemble a single instruction and advance the internal cursor
  auto singleStep() -> std::string;

private:

  u8 *mem_ = nullptr;
  u8 *cursor_ = nullptr;
};

}
