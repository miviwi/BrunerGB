#pragma once

#include <bus/bus.h>
#include <bus/device.h>
#include <sched/device.h>

#include <device/sm83/registers.h>
#include <device/sm83/instruction.h>

#include <memory>

namespace brgb::sm83 {

class Processor : public IBusDevice, public ISchedDevice {
public:
  using ProcessorBus = Bus<16>;

  auto connect(SystemBus *sys_bus) -> void;

  auto bus() -> ProcessorBus&;

  virtual auto deviceToken() -> DeviceToken = 0;

  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * = 0;
  virtual auto detach(DeviceMemoryMap *map) -> void = 0;

  virtual auto power() -> void;
  virtual auto main() -> void = 0;

protected:
  virtual auto read(u16 addr) -> u8 = 0;
  virtual auto write(u16 addr, u8 data) -> void = 0;

  auto instruction() -> void;

  // ops.cpp
  auto opHALT() -> void;

  std::unique_ptr<ProcessorBus> bus_;

private:
  auto opcode() -> u8;

  auto operand8() -> u8;
  auto operand16() -> u16;

  auto push16(u16 data) -> void;
  auto pop16() -> u16;

  // Return the value of the register specified by 'which'
  auto reg(Reg8 which) -> u8;
  auto reg(Reg16_rp which) -> u16;
  auto reg(Reg16_rp2 which) -> u16;

  // Set the register specified by 'which' to 'val'
  auto reg(Reg8 which, u8 val) -> void;
  auto reg(Reg16_rp which, u16 val) -> void;
  auto reg(Reg16_rp2 which, u16 val) -> void;

  auto alu(AluOp op, u8 val) -> void;
  auto rot(RotOp op, u8 val) -> void;
  auto akku(AkkuOp op) -> void;

  Registers r;
};

}
