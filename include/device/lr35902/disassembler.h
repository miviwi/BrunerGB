#pragma once

#include <types.h>
#include <util/format.h>

#include <stdexcept>
#include <string>

#include <cstddef>

namespace brgb::lr35902 {

class Disassembler {
public:
  struct IllegalOpcodeError final : public std::runtime_error {
    IllegalOpcodeError(ptrdiff_t offset, u8 op) :
      std::runtime_error(
          util::fmt("illegal opcode 0x%.2x@0x%.4llx", op, (unsigned long long)offset)
      )
    { }
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

  auto begin(u8 *mem) -> Disassembler&;

  auto singleStep() -> std::string;

private:
  u8 *mem_ = nullptr;
  u8 *cursor_ = nullptr;
};

}
