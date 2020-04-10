#pragma once

namespace brdrive {

// Forward declaration
class GLProgram;

namespace osd_detail {
auto init_DrawString_program() -> GLProgram*;
auto init_DrawRectangle_program() -> GLProgram*;
auto init_DrawShadedQuad_program() -> GLProgram*;
}

}
