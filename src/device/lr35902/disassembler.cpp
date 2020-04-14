#include <device/lr35902/disassembler.h>

#include <cassert>
#include <cstddef>
#include <utility>

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

inline auto lo_nibble(u8 op) -> bool { return op & 0x0F; }
inline auto hi_nibble(u8 op) -> bool { return op & 0xF0; }

template <typename... Args>
inline auto hi_nibble_matches(u8 op, Args... args) -> bool
{
  return ((hi_nibble(op) == args) || ...);
}

inline auto hi_nibble_between(u8 op, u8 min, u8 max) -> bool
{
  return hi_nibble(op) >= min && hi_nibble(op) <= max;
}

auto Instruction::decode(u8 *ptr) -> u8 *
{
  // Fetch the opcode
  u8 op = *ptr++;
  op_ = op;

  // Delegate to alternate method for CB-prefixed opcodes
  if(op == CB_prefix) return decodeCBPrefixed(ptr);

  if(hi_nibble_between(op, 0x00, 0x30)) {
    return decode0x00_0x30(ptr);
  } else if(hi_nibble_between(op, 0x40, 0x80)) {
    if(op == OpcodeHALT) {  // The HALT opcode is mixed-in with the LDs
      op_mnem_ = op_halt;
    } else {
      op_mnem_ = op_ld;
    }
  } else if(hi_nibble_between(op, 0x80, 0xB0)) {
    return decode0x80_0xB0(ptr);
  } else if(hi_nibble_between(op, 0xC0, 0xF0)) {
    return decode0xC0_0xF0(ptr);
  }

  return ptr;
}

auto Instruction::decode0x00_0x30(u8 *ptr) -> u8 *
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

        //  jr nz, r8
        std::make_pair((u8)0x20, [this,&ptr]() {
          op_mnem_ = op_jr;

          // Fetch the jump displacement
          operand_ = *ptr++;
        }),
        //  jr nc, r8
        std::make_pair((u8)0x30, [this,&ptr]() {
          op_mnem_ = op_jr;

          // Fetch the jump displacement
          operand_ = *ptr++;
        }));
      }),

      //  ld bc, <d16>
      //  ld de, <d16>
      //  ld hl, <d16>
      //  ld sp, <d16>
      std::make_pair((u8)0x01, [this,&ptr]() {
        op_mnem_ = op_ld;

        // Fetch the source address
        //   - Little-endian byte ordering
        operand_lo_ = *ptr++;
        operand_hi_ = *ptr++;
      }),
      //  ld (bc), A
      //  ld (de), A
      //  ld (hl+), A
      //  ld (hl-), A
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

      //  ld b, <d8>
      //  ld d, <d8>
      //  ld h, <d8>
      //  ld (hl), <d8>
      std::make_pair((u8)0x06, [this,&ptr]() {
        op_mnem_ = op_ld;
      }),
      //  ld c, <d8>
      //  ld e, <d8>
      //  ld l, <d8>
      //  ld a, <d8>
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
          //  ld (<a16>), SP
          std::make_pair((u8)0x00, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the source address
            //   - Little-endian byte ordering
            operand_lo_ = *ptr++;
            operand_hi_ = *ptr++;
          }),

          //  jr <r8>
          std::make_pair((u8)0x10, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the jump displacement
            operand_ = *ptr++;
          }),
          //  jr Z, <r8>
          std::make_pair((u8)0x20, [this,&ptr]() {
            op_mnem_ = op_ld;

            // Fetch the jump displacement
            operand_ = *ptr++;
          }),
          //  jr C, <r8>
          std::make_pair((u8)0x30, [this,&ptr]() {
            op_mnem_ = op_ld;

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

auto Instruction::decode0x80_0xB0(u8 *ptr) -> u8 *
{
  return ptr;
}

auto Instruction::decode0xC0_0xF0(u8 *ptr) -> u8 *
{
  assert(0 && "Instruction::decode0xC0_0xF0 stub!");
  return ptr;
}

auto Instruction::decodeCBPrefixed(u8 *ptr) -> u8 *
{
  op_CB_prefixed_ = true;

  // Discard the fetched prefix and fetch the real opcode
  u8 op = *ptr++;
  op_ = op;

  return ptr;
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
#define FMT_ARGS (unsigned)op, (unsigned)offset, Disassembler::opcode_to_str(op).data(), operand
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

auto Disassembler::opcode_to_str(u8 op) -> std::string
{
  

  return "<unknown>";
}

auto Disassembler::opcode_0xCB_to_str(u8 op) -> std::string
{
  return "<unknown>";
}

}
