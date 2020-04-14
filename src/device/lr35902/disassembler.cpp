#include <device/lr35902/disassembler.h>

#include <cstddef>

namespace brgb::lr35902 {

auto Disassembler::IllegalOperandError::format_error(
    ptrdiff_t offset, u8 op, OperandSize op_size, unsigned operand
  ) -> std::string
{
  return "";
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
