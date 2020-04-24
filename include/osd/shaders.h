#pragma once

namespace brgb {

// Forward declarations
class GLShader;
class GLProgram;

namespace osd_detail {
auto init_DrawString_program() -> GLProgram*;
auto init_DrawRectangle_program() -> GLProgram*;
auto init_DrawShadedQuad_program() -> GLProgram*;

// The returned pointer must NOT be freed by the caller
auto compile_DrawShadedQuad_vertex_shader() -> GLShader*;
}

}
