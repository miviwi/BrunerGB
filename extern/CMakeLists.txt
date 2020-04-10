set (ExternDir ${PROJECT_SOURCE_DIR}/extern)

target_sources (BrunerDrive PRIVATE
  # OpenGL/gl3w
  ${ExternDir}/GL/gl3w.c
)
