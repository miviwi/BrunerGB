#include <device/huc6280/disassembler.h>

#include <cassert>

namespace brgb::huc6280disasm {

Instruction(u8 *mem) :
  mem_(mem)
{
}

auto Instruction::disassemble(u8 *ptr) -> u8*
{
  return ptr;
}

auto Disassembler::begin(u8 *mem) -> Disassembler&
{
  // Setup the binary's address and the current instruction cursor
  mem_ = cursor_ = mem;

  return *this;
}

auto Disassembler::singleStep() -> std::string
{
  assert(0);
  return "";
}

}
