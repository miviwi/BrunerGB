#pragma once

#include <system/gb/cpu.h>

#include <bus/bus.h>
#include <bus/memorymap.h>

#include <memory>

namespace brgb {

class Gameboy {
public:
  Gameboy();

  // Connects all the devices to the SystemBus
  auto init() -> Gameboy&;

  auto power() -> void;

private:
  auto cpu() -> gb::CPU&;

  // Set to 'true' by init(), when all devices have
  //   been connected to the SystemBus
  bool was_init_ = false;

  std::unique_ptr<SystemBus> bus_;

  // All the system's devices
  std::unique_ptr<gb::CPU> cpu_;
};

}
