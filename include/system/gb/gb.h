#pragma once

#include <bus/bus.h>
#include <bus/memorymap.h>
#include <bus/mappedrange.h>
#include <sched/scheduler.h>

#include <system/gb/cpu.h>

#include <memory>
#include <array>

namespace brgb {

class Gameboy {
public:
  static constexpr double SystemClock = 4.0 * 1024*1024;   // ~4MHz

  Gameboy();

  // Connects all the devices to the SystemBus and
  //   creates all the device's Scheduler threads
  auto init() -> Gameboy&;

  auto power() -> void;

private:
  auto sysBus() -> SystemBus&;

  auto cpu() -> gb::CPU&;

  auto wramReadHandler() -> BusReadHandler::ByteHandler;
  auto wramWriteHandler() -> BusWriteHandler::ByteHandler;

  auto hramReadHandler() -> BusReadHandler::ByteHandler;
  auto hramWriteHandler() -> BusWriteHandler::ByteHandler;

  // Set to 'true' by init(), when all devices have
  //   been connected to the SystemBus
  bool was_init_ = false;

  Scheduler sched;

  std::unique_ptr<SystemBus> bus_;

  // All the system's devices
  std::unique_ptr<gb::CPU> cpu_;

  std::array<u8, 8192> wram_;
  std::array<u8, 128>  hram_;
};

}
