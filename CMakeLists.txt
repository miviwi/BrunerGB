cmake_minimum_required (VERSION 3.11)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set up the project and it's main executable
project (BrunerDrive)
add_executable (BrunerDrive)

find_package (X11 REQUIRED)
find_package (OpenGL REQUIRED)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "-std=c++17 -g")

target_include_directories (BrunerDrive PUBLIC ./include)
target_include_directories (BrunerDrive PRIVATE ./extern)
target_link_libraries (BrunerDrive PRIVATE
  # X11
  xcb X11-xcb xcb-util ${X11_LIBRARIES}

  # OpenGL
  GL

  ${CMAKE_DL_LIBS}  # OpenGL/gl3w dependency
)

add_subdirectory (./src)
add_subdirectory (./extern)

# For YouCompleteMe syntactic completion
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
endif ()
