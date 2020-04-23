#include "sched/scheduler.h"
#include "util/bit.h"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <brgb.h>
#include <memory>
#include <types.h>
#include <window/geometry.h>
#include <window/color.h>
#include <window/window.h>
#include <gx/gx.h>
#include <gx/extensions.h>
#include <gx/context.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/texture.h>
#include <gx/program.h>
#include <gx/pipeline.h>
#include <gx/fence.h>
#include <x11/x11.h>
#include <x11/connection.h>
#include <x11/window.h>
#include <x11/event.h>
#include <x11/glx.h>
#include <osd/osd.h>
#include <osd/font.h>
#include <osd/drawcall.h>
#include <osd/surface.h>
#include <device/sm83/cpu.h>
#include <device/sm83/registers.h>
#include <device/sm83/disassembler.h>
#include <bus/bus.h>
#include <bus/device.h>
#include <bus/memorymap.h>
#include <sched/device.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <GL/gl3w.h>

#include <libco/libco.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

using namespace brgb;

auto load_font(const std::string& file_name) -> std::optional<std::vector<uint8_t>>
{
  auto fd = open(file_name.data(), O_RDONLY);
  if(fd < 0) return std::nullopt;

  struct stat st;
  if(fstat(fd, &st) < 0) {
    close(fd);
    return std::nullopt;
  }

  std::vector<uint8_t> font(st.st_size);
  if(read(fd, font.data(), font.size()) < 0) {
    close(fd);
    return std::nullopt;
  }

  close(fd);
  return std::move(font);
}

auto load_bootrom() -> std::optional<std::vector<uint8_t>>
{
  auto fd = open("./boot.rom", O_RDONLY);
  if(fd < 0) return std::nullopt;

  struct stat st;
  if(fstat(fd, &st) < 0) {
    close(fd);
    return std::nullopt;
  }

  std::vector<uint8_t> bootrom(st.st_size);
  if(read(fd, bootrom.data(), bootrom.size()) < 0) {
    close(fd);
    return std::nullopt;
  }

  close(fd);
  return bootrom;
}

class TestCPU : public sm83::Processor {
public:
  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * final
  {
    auto map = bus->createMap(this);

    // Perform some device-specific initialization here for 'target'

    return map;
  }

  virtual auto detach(DeviceMemoryMap *map) -> void final
  {
  }

  virtual auto power() -> void
  {
    sm83::Processor::power();
  }

  virtual auto main() -> void
  {
    scheduler()->yield(ISchedDevice::Sync);
  }

protected:
  virtual auto read(u16 addr) -> u8
  {
    return bus().readByte(addr);
  }

  virtual auto write(u16 addr, u8 data) -> void
  {
    bus().writeByte(addr, data);
  }
};

class TestRAM : public IBusDevice {
public:
  virtual auto attach(SystemBus *bus, IBusDevice *target) -> DeviceMemoryMap * final
  {
    return bus->createMap(this);
  }

  virtual auto detach(DeviceMemoryMap *map) -> void final
  {
    assert(0);
  }

  auto readByteHandler() -> BusReadHandler::ByteHandler
  {
    return BusReadHandler::ByteHandler([this](auto addr) -> u8 {
        return readByte((u16)addr);
    });
  }

  auto writeByteHandler() -> BusWriteHandler::ByteHandler
  {
    return BusWriteHandler::ByteHandler([this](auto addr, u8 data) -> void {
        writeByte((u16)addr, data);
    });
  }

private:
  auto readByte(u16 address) -> u8
  {
    return ram_[address];
  }

  auto writeByte(u16 address, u8 data) -> void
  {
    ram_[address] = data;
  }

  u8 ram_[0x2000];
};

class TestSystem {
public:
  auto init() -> TestSystem&
  {
    bus = std::make_unique<SystemBus>();

    cpu = std::make_unique<TestCPU>();
    ram = std::make_unique<TestRAM>();

    bus->addressSpaceFactory([]() -> IAddressSpace * {
        return new AddressSpace<16>();
    });

    cpu->connect(bus.get());

    auto& cpu_ram = *cpu->attach(bus.get(), ram.get());

    cpu_ram
      .r("0x0000-0x1fff,0x4000-0x5fff", [this](BusTransactionHandlerSetRef& handler_set) {
          handler_set.get<BusReadHandlerSet>()
            .fn(ram->readByteHandler())
            .mask(0x1fff);
      })
      .w("0x0000-0x1fff,0x4000-0x5fff", [this](BusTransactionHandlerSetRef& handler_set) {
          handler_set.get<BusWriteHandlerSet>()
            .fn(ram->writeByteHandler())
            .mask(0x1fff);
      });

   auto cpu_bus = cpu->bus();

   cpu_bus.writeByte(0x0000, 0x12);
   cpu_bus.writeByte(0x4001, 0x34);

   printf("cpu_bus@0x0000 = 0x%.2x\n",  (unsigned)cpu_bus.readByte(0x0000));
   printf("cpu_bus@0x0001 = 0x%.2x\n",  (unsigned)cpu_bus.readByte(0x0001));

#if 0
   auto test_lookup = [&](u16 addr) {
     printf("lookup(0x%.4x) -> %p\n", addr, cpu_ram.lookupR(addr));
   };

   test_lookup(0x0000);
   test_lookup(0x100f);
   test_lookup(0x1fff);
   test_lookup(0x2000);
   test_lookup(0x4000);
   test_lookup(0x6000);
#endif

    return *this;
  }

  std::unique_ptr<SystemBus> bus;

  std::unique_ptr<TestCPU> cpu;
  std::unique_ptr<TestRAM> ram;
};

auto test_cpu() -> void
{
  TestSystem test_system;

  test_system.init();


}

auto test_disasm() -> void
{
  auto bootrom = load_bootrom();

  sm83::Disassembler disasm;

  disasm.begin(bootrom->data());

  for(int i = 0; i < 35; i++) {
    printf(disasm.singleStep().data());
  }
}

int main(int argc, char *argv[])
{
  x11_init();

  X11Window window;
  X11EventLoop event_loop;

  Geometry window_geometry = { 0, 0, 256, 256 };

  window
    .geometry(window_geometry)
    .background(Color(1.0f, 0.0f, 1.0f, 0.0f))
    .create()
    .show();

  event_loop
    .init(&window);

//  test_cpu();
//  test_disasm();
//  return 0;

  GLXContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  gx_init();
  osd_init();

#if 0
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  auto pipeline = GLPipeline()
    .add<GLPipeline::Viewport>(1280, 720);

  pipeline.use();

  gl_context
    .dbg_EnableMessages();

  printf("OpenGL %s\n\n", gl_context.versionString().data());

  gl_context
    .dbg_PushCallGroup("OSD");

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  glClearColor(1.0f, 1.0f, 0.0f, 0.5f);

  auto topaz_1bpp = load_font("Topaz.raw");
  if(!topaz_1bpp) {
    puts("couldn't load font file `Topaz.raw'!");
    return -1;
  }

  auto topaz = OSDBitmapFont().loadBitmap1bpp(topaz_1bpp->data(), topaz_1bpp->size());
  printf("topaz_1bpp.size()=%zu  topaz.size()=%zu\n", topaz_1bpp->size(), topaz.pixelDataSize());
  fflush(stdout);

  auto c = x11().connection<xcb_connection_t>();

  OSDSurface some_surface;
  some_surface
    .create({ window_geometry.w, window_geometry.h }, &topaz)
    .writeString({ 0, 30 }, "hello, world!", Color::red())
    .writeString({ 0, 0 }, "ASDF1234567890", Color::red())
    .writeString({ 128, 100 }, "xyz", Color::blue())
    .writeString({ 128, 200 }, "!#@$", Color::green());

  glViewport(0, 0, window_geometry.w, window_geometry.h);
  
  bool running = true;
  bool change = false;
  bool use_fence = false;
  while(running) {
    auto ev = event_loop.event();
    bool use_fence_initial = use_fence;

    switch(ev ? ev->type() : Event::Invalid) {
    case Event::KeyDown: {
      auto event = (IKeyEvent *)ev.get();

      auto code = event->code();
      auto sym  = event->sym();

      printf("code=(0x%2X %3u, %c) sym=(0x%2X %3u, %c)\n",
          code, code, code, sym, sym, sym);

      if(sym == 'q') running = false;

      if(sym == 'f') use_fence = !use_fence;
      break;
    }

    case Event::MouseMove: {
      auto event = (IMouseEvent *)ev.get();
      auto delta = event->delta();

      break;
    }

    case Event::MouseDown: {
      auto event = (IMouseEvent *)ev.get();
      auto point = event->point();
      auto delta = event->delta();

      printf("click! @ (%hd, %hd) delta=(%hd, %hd)\n",
          point.x, point.y, delta.x, delta.y);

      window.drawString("hello, world!",
          Geometry::xy(point.x, point.y), Color::white()); 

      break;
    }

    case Event::Quit:
      running = false;
      break;
    }

    gl_context.dbg_PushCallGroup("OSD.some_surface");

    glClear(GL_COLOR_BUFFER_BIT);

    /*
    auto drawcall = osd_drawcall_strings(
        &vertex_array, GLType::u16, &text_index_buf, 0,
        14, 2,
        &topaz_tex, &topaz_tex_sampler,
        &string_tex_buf
    );
        
    osd_submit_drawcall(gl_context, drawcall);
    */

    auto surface_drawcalls = some_surface.draw();
    for(auto& drawcall : surface_drawcalls) {
      osd_submit_drawcall(gl_context, drawcall);
    }

    std::chrono::high_resolution_clock clock;
    auto start = clock.now();

    gl_context.swapBuffers();

    auto end = clock.now();

    auto ms_counts = std::chrono::milliseconds(1).count(); 

    if(use_fence_initial != use_fence && !use_fence) {
      printf("\nswapBuffers() without blocking on a fence took: %ldus\n\n",
          std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
    } else if(use_fence_initial) {
      printf("\nswapBuffers() AFTER BLOCKING on a fence took: %ldus\n\n",
          std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
    }

    gl_context.dbg_PopCallGroup();

    if(!running) break;
  }

  glPopDebugGroup();

  gl_context
    .destroy();

  window
     .destroy();

  x11_finalize();

  return 0;
}
