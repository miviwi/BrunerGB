#pragma once

#include <bus/bus.h>
#include <bus/device.h>
#include <sched/device.h>

#include <device/sm83/registers.h>

#include <memory>

namespace brgb::sm83 {

class Processor : public IBusDevice, public ISchedDevice {
public:
  using ProcessorBus = Bus<16>;

  auto connect(SystemBus *sys_bus) -> void;

  auto bus() -> ProcessorBus&;

  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;

  virtual auto power() -> void;
  virtual auto main() -> void = 0;

protected:
  virtual auto read(u16 addr) -> u8 = 0;
  virtual auto write(u16 addr, u8 data) -> void = 0;

  std::unique_ptr<ProcessorBus> bus_;

private:
  auto operand8() -> u8;
  auto operand16() -> u16;

  auto push16(u16 data) -> void;
  auto pop16() -> u16;

  auto op_nop() -> void;                                     // nop
  auto op_stop() -> void;                                    // stop 0
  auto op_jr(u1 cond) -> void;                               // jr <cond>, <rel8>
  auto op_ld_Imm16(Natural<16>& reg) -> void;                // ld <reg16>, <imm16>
  auto op_ld_Indirect16(Natural<16>& ptr, u8 data) -> void;  // ld (<reg16>), a
  auto op_ldi(u8 data) -> void;                              // ld (hl+), a
  auto op_ldd(u8 data) -> void;                              // ld (hl-), a
  auto op_inc_Reg16(Natural<16>& reg) -> void;               // inc <reg16>
  auto op_dec_Reg16(Natural<16>& reg) -> void;               // dec <reg16>

  Registers r;
};

}
