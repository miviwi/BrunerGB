#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <brdrive.h>
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

int main(int argc, char *argv[])
{
  using namespace brdrive;
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

  gl_context.dbg_PushCallGroup("Compute");

  GLProgram compute_shader_program;

  GLShader compute_shader_program_shader(GLShader::Compute);
  compute_shader_program_shader
    .glslVersion(430)
    .source(R"COMPUTE(
uniform writeonly image2D uiComputeOut;

uniform float ufWavePeriod;

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

void main()
{
  float work_group_x = float(gl_WorkGroupID.x) / 4096.0f;     // normalize to [0;1]

  float wave_sin = sin(work_group_x * ufWavePeriod*(1.0f/2.0f));
  float wave_cos = cos(work_group_x * ufWavePeriod);

  float blue = (wave_sin < 0.0f) && (wave_cos < 0.0f) ? 1.0f : 0.0f;

  vec2 wave = pow(vec2(wave_sin, wave_cos), vec2(2.0f));

  imageStore(uiComputeOut, ivec2(int(gl_WorkGroupID.x), 0), vec4(wave, blue, 1));
}
)COMPUTE");

  try {
    compute_shader_program_shader.compile();
  } catch(const std::exception& e) {
    auto info_log = compute_shader_program_shader.infoLog();

    if(info_log) {
      puts(info_log->data());
    }

    return -2;
  }

  compute_shader_program_shader.label("p.ComputeCS");

   compute_shader_program
     .attach(compute_shader_program_shader);

   try {
    compute_shader_program.link();
  } catch(const std::exception& e) {
    if(!compute_shader_program.infoLog()) return -2;

    puts(compute_shader_program.infoLog()->data());
    return -2;
  }

  GLTexture2D compute_output_tex;
  compute_output_tex
    .alloc(4096, 1, 1, rgba8);

  compute_output_tex.label("t2d.ComputeOutput");

  glBindImageTexture(0, compute_output_tex.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  compute_shader_program.label("p.Compute");

  compute_shader_program
    .uniform("uiComputeOut", gl_context.texImageUnit(0))
    .uniform("ufWavePeriod", 1024.0f);

  std::chrono::high_resolution_clock clock;
  auto start = clock.now();

  compute_shader_program.use();
  glDispatchCompute(4096, 1, 1);

  auto end = clock.now();

  auto ms_counts = std::chrono::milliseconds(1).count(); 

  printf("\ncompute_shader_program took: %ldus\n\n",
      std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());

  GLFence compute_fence;
  compute_fence
    .fence()
    .label("f.Compute")
    .block();

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
