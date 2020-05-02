#include "sched/scheduler.h"
#include "util/bit.h"
#include <cinttypes>
#include <cstddef>
#include <cstdint>
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
#include <osd/pixmap.h>
#include <osd/quadshader.h>
#include <osd/drawcall.h>
#include <osd/surface.h>
#include <device/sm83/cpu.h>
#include <device/sm83/registers.h>
#include <device/sm83/disassembler.h>
#include <device/huc6280/disassembler.h>
#include <bus/bus.h>
#include <bus/device.h>
#include <bus/memorymap.h>
#include <sched/device.h>
#include <system/gb/gb.h>

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

auto test_system() -> void
{
  Gameboy gb;

  gb
    .init()
    .power();
}

auto test_disasm() -> void
{
  std::vector<u8> binary = {
    0x00,          // BRK
    0xEA,          // NOP
    0xA9, 0x69,    // LDA #$69
    0x47, 0xEF,    // RMB4 $EF
    0xD3, 0x00, 0xFF, 0x00, 0x10, 0x00, 0x01, // TIN $FF00, $1000, $100
    0x80, 0xF2,    // LDA #$69
  };

  huc6280disasm::Disassembler disasm;

  disasm.begin(binary.data());

  for(int i = 0; i < 6; i++) {
    printf(disasm.singleStep().data());
  }
}

int main(int argc, char *argv[])
{
  test_disasm();
  return 0;

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
  test_system();
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

  OSDPixmap osd_bitmap(160, 144);

  {
    auto bitmap_view = osd_bitmap.lock();
    for(auto& c : bitmap_view) {
      c = OSDPixmap::Color { 0xFF, 0x66, 0x66, 0xFF };
    }
    osd_bitmap.unlock();
  }
  {
    auto bitmap_view = osd_bitmap.lock();
    for(auto& c : bitmap_view) {
      c = OSDPixmap::Color { 0xFF, 0xFF, 0x66, 0xFF };
    }
    osd_bitmap.unlock();
  }


  OSDSurface some_surface;
  some_surface
    .create({ window_geometry.w, window_geometry.h }, &topaz)
    .writeString({ 0, 0 }, "ASDF1234567890", Color::red())
    .writeString({ 128, 100 }, "xyz", Color::blue())
    .writeString({ 128, 200 }, "!#@$", Color::green());

    auto& qs = some_surface.createShader();

    qs.addPixmapArray("usFrameHistory", 2); 

    qs
      .addFunction("vec4 computeFragColor()",
R"FRAG(
  return vec4(fi.UV, 0.0f, 1.0f);
)FRAG")
      .entrypoint("computeFragColor");

  auto& qs_program = qs.program();

  some_surface
    .drawQuad({ 50, 35 }, { 50, 70 }, &qs, {})
    .writeString({ 0, 30 }, "hello, world!", Color::red());

  glViewport(0, 0, window_geometry.w, window_geometry.h);
  
  bool running = true;
  bool change = false;
  bool use_fence = true;
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

#if 0
    if(use_fence_initial != use_fence && !use_fence) {
      printf("\nswapBuffers() without blocking on a fence took: %ldus\n\n",
          std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
    } else if(use_fence_initial) {
      printf("\nswapBuffers() AFTER BLOCKING on a fence took: %ldus\n\n",
          std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
    }
#endif

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
