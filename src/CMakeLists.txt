set (SrcDir ${PROJECT_SOURCE_DIR}/src)

target_sources (BrunerGB PRIVATE
  ${SrcDir}/brgb.cpp

  # Utility function sources
  ${SrcDir}/util/format.cpp

  # Window specific sources
  ${SrcDir}/window/window.cpp
  ${SrcDir}/window/event.cpp

  # OpenGL
  ${SrcDir}/gx/gx.cpp
  ${SrcDir}/gx/context.cpp
  ${SrcDir}/gx/extensions.cpp
  ${SrcDir}/gx/object.cpp
  ${SrcDir}/gx/pipeline.cpp
  ${SrcDir}/gx/vertex.cpp
  ${SrcDir}/gx/buffer.cpp
  ${SrcDir}/gx/texture.cpp
  ${SrcDir}/gx/program.cpp
  ${SrcDir}/gx/fence.cpp
  ${SrcDir}/gx/handle.cpp

  # X11 specific sources
  ${SrcDir}/x11/x11.cpp
  ${SrcDir}/x11/connection.cpp
  ${SrcDir}/x11/window.cpp
  ${SrcDir}/x11/event.cpp
  ${SrcDir}/x11/glx.cpp

  # OSD specific sources
  ${SrcDir}/osd/osd.cpp
  ${SrcDir}/osd/font.cpp
  ${SrcDir}/osd/pixmap.cpp
  ${SrcDir}/osd/shaders.cpp
  ${SrcDir}/osd/quadshader.cpp
  ${SrcDir}/osd/util.cpp
  ${SrcDir}/osd/drawcall.cpp
  ${SrcDir}/osd/surface.cpp

  # System bus
  ${SrcDir}/bus/bus.cpp
  ${SrcDir}/bus/device.cpp
  ${SrcDir}/bus/memorymap.cpp
  ${SrcDir}/bus/mappedrange.cpp

  # Scheduler sources
  ${SrcDir}/sched/scheduler.cpp
  ${SrcDir}/sched/device.cpp
  ${SrcDir}/sched/thread.cpp

  # Device sources
  #   SM83
  ${SrcDir}/device/sm83/cpu.cpp
  ${SrcDir}/device/sm83/registers.cpp
  ${SrcDir}/device/sm83/instruction.cpp
  ${SrcDir}/device/sm83/ops.cpp
  ${SrcDir}/device/sm83/disassembler.cpp
  #   HuC6280
  ${SrcDir}/device/huc6280/cpu.cpp
  ${SrcDir}/device/huc6280/registers.cpp
  ${SrcDir}/device/huc6280/disassembler.cpp

  # System sources
  #   Gameboy
  ${SrcDir}/system/gb/gb.cpp
  ${SrcDir}/system/gb/cpu.cpp
)
