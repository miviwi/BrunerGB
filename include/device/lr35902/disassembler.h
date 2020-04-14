#pragma once

#include <array>
#include <types.h>
#include <util/format.h>
#include <util/natural.h>
#include <util/bit.h>

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
  op_ld,
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
  static constexpr u8 CB_prefix = 0xCB;

  // Populates the Instruction object with data at 'ptr'
  //   and returns 'ptr' advanced appropriately i.e. by
  //   the width of the opcode and it's operands (if any)
  auto decode(u8 *ptr) -> u8 *;

private:
  auto decode0x00_0x30(u8 *ptr) -> u8 *;
  auto decode0x80_0xB0(u8 *ptr) -> u8 *;
  auto decode0xC0_0xF0(u8 *ptr) -> u8 *;
  auto decodeCBPrefixed(u8 *ptr) -> u8 *;

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
    (((op & 0x0F) == handlers.first && (handlers.second(), 0)), ...);
  }

  template <typename... Fn>
  auto dispatchOnHiNibble(std::pair<u8 /* hi_nibble */, Fn>... handlers) -> void
  {
    u8 op = op_;

    (((op & 0xF0) == handlers.first && (handlers.second(), 0)), ...);
  }

  u8 *mem_ = nullptr;

  bool op_CB_prefixed_ = false;
  Opcode op_;
  OpcodeMnemonic op_mnem_ = op_Invalid;

  Natural<16> operand_ = 0;
  BitRange<16, 0, 7> operand_lo_{ operand_.ptr() };
  BitRange<16, 8, 15> operand_hi_{ operand_.ptr() };
};

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

  // Returns an opcode's mnemonic
  static auto opcode_to_str(u8 op) -> std::string;

  // Returns the mnemonic for a CB-prefixed opcode
  static auto opcode_0xCB_to_str(u8 op) -> std::string;

private:

  u8 *mem_ = nullptr;
  u8 *cursor_ = nullptr;
};

}
