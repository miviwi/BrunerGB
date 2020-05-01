#pragma once

#include <types.h>

#include <limits>
#include <string>

#include <cstddef>

namespace brgb::huc6280disasm {

class Instruction {
public:
  // 'mem' is a pointer to the base of the binary being diassembled
  Instruction(u8 *mem);

  // Populates the Instruction object with data at 'ptr'
  //   and returns 'ptr' advanced appropriately i.e. by
  //   the width of the opcode and it's operands (if any)
  auto disassemble(u8 *ptr) -> u8*;

private:
  // Base address of the binary being diassembled
  u8 *mem_ = nullptr;
  // Offset of this instruction in the binary (i.e. relative to 'mem_')
  uintptr_t offset_ = std::numeric_limits<uintptr_t>::max();

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
