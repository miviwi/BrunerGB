#include <system/gb/gb.h>
#include <system/gb/cpu.h>

#include <sched/thread.h>

#include <cassert>

namespace brgb {

Gameboy::Gameboy() :
  bus_(new SystemBus()),

  cpu_(new gb::CPU())
{
}

auto Gameboy::init() -> Gameboy&
{
  // Initialize the SystemBus
  sysBus()
    .addressSpaceFactory([](IBusDevice::DeviceToken dev_token) -> IAddressSpace* {
        switch(dev_token) {
        case gb::CPU::GameboyCPUDeviceToken: return new AddressSpace<16>;
        }

        assert(0);      // Unreachable
        return nullptr;
    });

  // Create the Bus(es) and map all the devices
  //   - CPU bus
  cpu().connect(bus_.get());

  auto& cpu_ram = *cpu().attach(bus_.get());

  cpu_ram
    .r("0xc000-0xdfff,0xe000-0xfdff", [this](BusTransactionHandlerSetRef& h) {
        h.get<BusReadHandlerSet>()
          .fn(wramReadHandler())
          .mask(0x1FFF);
    })
    .w("0xc000-0xdfff,0xe000-0xfdff", [this](BusTransactionHandlerSetRef& h) {
        h.get<BusWriteHandlerSet>()
          .fn(wramWriteHandler())
          .mask(0x1FFF);
    })
    
    .r("0xff80-0xfffe", [this](BusTransactionHandlerSetRef& h) {
        h.get<BusReadHandlerSet>()
          .fn(hramReadHandler())
          .base(0x0080)
          .mask(0x007f);
    })
    .w("0xff80-0xfffe", [this](BusTransactionHandlerSetRef& h) {
        h.get<BusWriteHandlerSet>()
          .fn(hramWriteHandler())
          .base(0x0080)
          .mask(0x007f);
    });

  // Call Thread::create() for all of the device threads
  sched.add(Thread::create(SystemClock, cpu_.get()));
  // TODO: create threads for the rest of the devices

  was_init_ = true;

  return *this;
}

auto Gameboy::power() -> void
{
  assert(was_init_ && "init() MUST be called before power()!");

  cpu().power();
  // TODO: power up the rest of the devices

  // Make the CPU the primary device
  sched.power(cpu_.get());
}

auto Gameboy::sysBus() -> SystemBus&
{
  return *bus_;
}

auto Gameboy::cpu() -> gb::CPU&
{
  return *cpu_;
}

auto Gameboy::wramReadHandler() -> BusReadHandler::ByteHandler
{
  return BusReadHandler::for_u8_with_addr_width<u16>([this](u16 addr) {
      assert(addr < wram_.size());    // Sanity check

      return wram_[addr];
  });
}

auto Gameboy::wramWriteHandler() -> BusWriteHandler::ByteHandler
{
  return BusWriteHandler::for_u8_with_addr_width<u16>([this](u16 addr, u8 data) {
      assert(addr < wram_.size());    // Sanity check

      wram_[addr] = data;
  });
}

auto Gameboy::hramReadHandler() -> BusReadHandler::ByteHandler
{
  return BusReadHandler::for_u8_with_addr_width<u16>([this](u16 addr) {
      assert(addr < hram_.size());    // Sanity check

      return hram_[addr];
  });
}

auto Gameboy::hramWriteHandler() -> BusWriteHandler::ByteHandler
{
  return BusWriteHandler::for_u8_with_addr_width<u16>([this](u16 addr, u8 data) {
      assert(addr < hram_.size());    // Sanity check

      hram_[addr] = data;
  });
}

}
