#include "util/bit.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <brgb.h>
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
#include <device/lr35902/cpu.h>
#include <device/lr35902/registers.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xcb/xcb.h>
#include <X11/keysymdef.h>

#include <GL/gl3w.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>

auto load_font(const std::string& file_name) -> std::optional<std::vector<uint8_t>>
{
  auto fd = open(file_name.data(), O_RDONLY);
  if(fd < 0) return std::nullopt;

  struct stat st;
  if(fstat(fd, &st) < 0) return std::nullopt;

  std::vector<uint8_t> font(st.st_size);
  if(read(fd, font.data(), font.size()) < 0) {
    close(fd);
    return std::nullopt;
  }

  close(fd);
  return std::move(font);
}

auto test_cpu() -> void
{
  using namespace brgb;

  lr35902::Registers regs;

  using ZF = Bit<16, 7>;

  printf("u15::Mask=%x\n"
      "Bit<16, 7>::Width=%llx Bit<16, 7>::Mask=%llx\n",
      (unsigned)u15::Mask,
      ZF::Width, ZF::Mask);
}

int main(int argc, char *argv[])
{
  using namespace brgb;
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

  test_cpu();

  GLXContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  gx_init();
  osd_init();

  GLPipeline pipeline;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  gl_context
    .dbg_EnableMessages();

  printf("OpenGL %s\n\n", gl_context.versionString().data());

  gl_context
    .dbg_PopCallGroup()
    .dbg_PushCallGroup("OSD");

  glDisable(GL_DEPTH_TEST);

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

  glEnable(GL_PRIMITIVE_RESTART);
  glPrimitiveRestartIndex(0xFFFF);

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
  while(auto ev = event_loop.event(IEventLoop::Block)) {
    bool use_fence_initial = use_fence;

    switch(ev->type()) {
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
