#include <osd/shaders.h>

#include <gx/program.h>

#include <memory>

#include <cassert>
#include <cstdio>

namespace brgb::osd_detail {

static const char *s_osd_drawstring_vs_src = R"VERT(
#if defined(USE_INSTANCE_ATTRIBUTES)
layout(location = 0) in ivec4 viStringXYOffsetLength;
layout(location = 1) in vec4 viStringColorRGBX;
#endif

out Vertex {
  vec3 Position;
  vec3 Color;
  vec2 UV;
  float Character;
} vo;

// The positions of a single glyph's vertices
//   at the screen's top-left corner
const vec4 GlyphVertPositions[4] = vec4[](
  vec4(0.0f,  0.0f, 0.0f, 1.0f),
  vec4(0.0f, 16.0f, 0.0f, 1.0f),
  vec4(8.0f, 16.0f, 0.0f, 1.0f),
  vec4(8.0f,  0.0f, 0.0f, 1.0f)
);

// UV coordinates which encompass
//   a single character in 'usFont'
const vec2 UVs[4] = vec2[](
  vec2(0.0f, 0.0f/256.0f /* <usFont height>/<glyph height> -> 4096/16 -> 256 */),
  vec2(0.0f, 1.0f/256.0f),
  vec2(1.0f, 1.0f/256.0f),
  vec2(1.0f, 0.0f/256.0f)
);

uniform mat4 um4Projection;

uniform isamplerBuffer usStrings;

struct StringAttributes {
  vec2 position;
  int offset, length;

  vec3 color;
};

#if defined(USE_INSTANCE_ATTRIBUTES)
StringAttributes FetchStringAttributes(int string_offset)
{
  StringAttributes attrs;

  attrs.position = vec2(viStringXYOffsetLength.xy);
  attrs.offset = viStringXYOffsetLength.z;
  attrs.length = viStringXYOffsetLength.w;

  attrs.color = viStringColorRGBX.rgb;

  return attrs;
}
#else
uniform isamplerBuffer usStringAttributes;
uniform int uiStringAttributesBaseOffset;

// Fetch the string's properties from a texture, that is:
//   * position (expressed in pixels with 0,0 at the top left corner)
//   * the offset in the usStrings texture at which the string's
//     characters can be found
//   * the string's length
//   * the string's color (which could be packed better, but for now
//     24-bits per string are wasted)
//   * 16-bits of padding
//  and unpack them for convenient access
StringAttributes FetchStringAttributes(int string_offset)
{
  StringAttributes attrs;

  int texel_off = uiStringAttributesBaseOffset + string_offset*2;

  ivec4 packed0 = texelFetch(usStringAttributes, texel_off+0);
  ivec4 packed1 = texelFetch(usStringAttributes, texel_off+1);

  attrs.position = vec2(packed0.xy);
  attrs.offset = packed0.z;
  attrs.length = packed0.w;

  attrs.color = vec3(packed1.rgb) * (1.0f/255.0f);    // Normalize

  return attrs;
}
#endif

// Gives an integer which is the index of the glyph
//   currently being rendered
int OffsetInString() { return gl_VertexID >> 2; }

// Gives an integer in the range [0;3] which is an
//   index of the current glyph's quad vertex
//   starting from the top-left and advancing
//   counter-clockwise
int GlyphQuad_VertexID() { return gl_VertexID & 3; }

const float TexCharHeight = 255.0f/256.0f;

void main()
{
  int string_character_num = OffsetInString();
  int vert_id = GlyphQuad_VertexID();

  StringAttributes attrs = FetchStringAttributes(gl_InstanceID);

  // Because of instancing, more characters can be rendered
  //   than there are in a given string, in the above case
  //   cull the additional glyphs
  if(string_character_num >= attrs.length) {
    gl_Position = vec4(0.0f, 0.0f, 0.0f, -1.0f);
    return;
  }

  // The index of the string's character being rendered
  int character_num = attrs.offset + string_character_num;

  int character = texelFetch(usStrings, character_num).r;
  float char_t_offset = float(character) * TexCharHeight;

  // Compute the offset of the glyph being rendered relative to the start of the string
  vec2 glyph_advance = vec2(float(string_character_num) * 8.0f, 0.0f);

  // Compute the needed output data...
  vec4 pos = GlyphVertPositions[vert_id];
  vec2 uv = UVs[vert_id] - vec2(0.0f, char_t_offset);
  vec4 projected_pos = um4Projection * (pos + vec4(attrs.position + glyph_advance, 0.0f, 0.0f));

  // ...and assign it
  vo.Position = projected_pos.xyz;
  vo.Color = attrs.color;
  vo.UV = uv;
  vo.Character = character;

  // Position the vertex according to the given offset
  //   and posiiton in the string being rendered
  gl_Position = projected_pos;
}
)VERT";

static const char *s_osd_drawstring_fs_src = R"FRAG(
in Vertex {
  vec3 Position;
  vec3 Color;
  vec2 UV;
  float Character;
} fi;

#if defined(NO_BLEND)
#  define OUTPUT_CHANNELS vec3
#else
#  define OUTPUT_CHANNELS vec4
#endif
out OUTPUT_CHANNELS foFragColor;

uniform sampler2D usFont;

void main()
{
  const float Radius = 0.5f;

  const vec3 Red    = vec3(1.0f, 0.0f, 0.0f);
  const vec3 Blue   = vec3(0.08f, 0.4f, 0.75f);
  const vec3 LBlue  = vec3(0.4f, 0.7f, 0.96f);
  const vec3 Yellow = vec3(1.0f, 1.0f, 0.0f);
  const vec3 Lime   = vec3(0.75f, 1.0f, 0.0f);
  const vec3 Orange = vec3(1.0f, 0.4f, 0.0f);
  const vec3 Black  = vec3(0.0f, 0.0f, 0.0f);
  const vec3 White  = vec3(1.0f, 1.0f, 1.0f);

  float glyph_sample = texture(usFont, fi.UV).r;
  float alpha = glyph_sample;

  vec3 glyph_color = fi.Color * glyph_sample;

#if defined(NO_BLEND)
  // Do the equivalent of an old-school alpha test (NOT - blend!)
  if(alpha < 1.0f) discard;

  foFragColor = glyph_color;
#else
  foFragColor = vec4(glyph_color, alpha);
#endif
}
)FRAG";

static const char *s_osd_drawquad_vs_src = R"VERT(
out Vertex {
  vec2 UV;
} vo;

// ivec4(pos.x, pos.y, width, height)
uniform ivec4 uv4Quad_Pos_Dimensions;

uniform mat4 um4Projection;

const vec2 UVs[4] = vec2[](
  vec2(0.0f, 0.0f),
  vec2(0.0f, 1.0f),
  vec2(1.0f, 1.0f),
  vec2(1.0f, 0.0f)
);

// Gives an integer in the range [0;3] which is an
//   index of the quad's vertex starting from the
//   top-left and advancing counter-clockwise
int GlyphQuad_VertexID() { return gl_VertexID & 3; }

void main()
{
  int vert_id = GlyphQuad_VertexID();

  vec2 pos = vec2(uv4Quad_Pos_Dimensions.xy);
  vec2 dims = vec2(uv4Quad_Pos_Dimensions.zw);

  vec2 QuadVerts[4] = vec2[](
    pos,                            // Top-left
    pos+vec2(0.0f, dims.y),         // Bottom-left
    pos+dims,                       // Bottom-right
    pos+vec2(dims.x, 0.0f)          // Top-right
  );

  vec2 vert = QuadVerts[vert_id];
  vec2 uv = UVs[vert_id];

  vo.UV = uv;

  gl_Position = um4Projection * vec4(vert, 0.0f, 1.0f);
}
)VERT";

static const char *s_osd_drawquad_fs_src = R"FRAG(
in Vertex {
  vec2 UV;
} fi;

#define OUTPUT_CHANNELS vec4

out OUTPUT_CHANNELS foFragColor;

void main()
{
  foFragColor = vec4(fi.UV, 0.0f, 1.0f);
}
)FRAG";

auto init_DrawString_program() -> GLProgram*
{
  auto gl_program_ptr = new GLProgram();
  auto& gl_program = *gl_program_ptr;

  GLShader vert(GLShader::Vertex);
  GLShader frag(GLShader::Fragment);

  vert
    .source(s_osd_drawstring_vs_src);

  frag
    .source(s_osd_drawstring_fs_src);

  auto try_compile_shader = [](GLShader& shader)
  {
    try {
      shader.compile();
    } catch(const std::exception& e) {
      // Print the compilation errors to the console...
      auto info_log = shader.infoLog();
      if(info_log && !info_log->empty()) puts(info_log->data());

      // ...and terminate
      exit(-2);
    }
  };

  try_compile_shader(vert);
  try_compile_shader(frag);

  vert.label("p.OSD.DrawStringVS");
  frag.label("p.OSD.DrawStringFS");

  gl_program
    .attach(vert)
    .attach(frag);

  gl_program.label("p.OSD.DrawString");

  // Analogous to try_compile_shader()
  try {
    gl_program.link();
  } catch(const std::exception& e) {
    auto info_log = gl_program.infoLog();
    if(info_log && !info_log->empty()) puts(info_log->data());

    exit(-2);
  }

  gl_program
    .detach(frag)
    .detach(vert);

  return gl_program_ptr;
}

auto init_DrawRectangle_program() -> GLProgram*
{
  puts("TODO: OSDDrawCall::DrawRectangle program unimplemented!");

  return nullptr;
}

auto init_DrawShadedQuad_program() -> GLProgram*
{
  return nullptr;
}

thread_local GLShader *p_quad_vert_shader = nullptr;

auto compile_DrawShadedQuad_vertex_shader() -> GLShader*
{
  if(p_quad_vert_shader) return p_quad_vert_shader;

  auto vert_ptr = new GLShader(GLShader::Vertex);
  auto& vert = *vert_ptr;

  vert
    .source(s_osd_drawquad_vs_src);

  try {
    vert.compile();
  } catch(const std::exception& e) {
    // Print the compilation errors to the console...
    auto info_log = vert.infoLog();
    if(info_log && !info_log->empty()) puts(info_log->data());

    // ...and terminate
    exit(-2);
  }

  vert.label("p.OSD.DrawShadedQuadVS");

  return p_quad_vert_shader = vert_ptr;
}

}
