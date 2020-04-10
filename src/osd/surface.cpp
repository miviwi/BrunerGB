#include <osd/surface.h>
#include <osd/font.h>
#include <osd/drawcall.h>

#include <gx/gx.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/texture.h>
#include <gx/buffer.h>

#include <cassert>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <utility>

namespace brdrive {

// Initialized during osd_init()
GLProgram **OSDSurface::s_surface_programs = nullptr;

OSDSurface::OSDSurface() :
  dimensions_(ivec2::zero()), font_(nullptr), bg_(Color::transparent()),
  created_(false),
  surface_object_inds_(nullptr), font_tex_(nullptr), font_sampler_(nullptr),
  strings_buf_(nullptr), strings_tex_(nullptr),
  string_attrs_buf_(nullptr), string_attrs_tex_(nullptr)
{
}

OSDSurface::~OSDSurface()
{
  destroyGLObjects();
}

auto OSDSurface::create(
    ivec2 width_height, const OSDBitmapFont *font, const Color& bg
  ) -> OSDSurface&
{
  assert((width_height.x > 0) && (width_height.y > 0) &&
      "width and height must be positive integers!");
  assert(s_surface_programs &&
      "osd_init() MUST be called prior to creating any OSDSurfaces!");

  dimensions_ = width_height;
  font_ = font;
  bg_ = bg;

  initGLObjects();

  created_ = true;

  return *this;
}

auto OSDSurface::writeString(ivec2 pos, const char *string, const Color& color) -> OSDSurface&
{
  assert(string && "attempted to write a nullptr string!");

  // Perform internal state validation
  if(!created_) throw NullSurfaceError();
  if(!font_) throw FontNotProvidedError();

  string_objects_.push_back(StringObject {
    pos, std::string(string), color,
  });

  return *this;
}

auto OSDSurface::draw() -> std::vector<OSDDrawCall>
{
  std::vector<OSDDrawCall> drawcalls;

  appendStringDrawcalls(drawcalls);

  return std::move(drawcalls);
}

auto OSDSurface::clear() -> OSDSurface&
{
  string_objects_.clear();

  return *this;
}

auto OSDSurface::renderProgram(int draw_type) -> GLProgram&
{
  assert(s_surface_programs &&
      "attempted to render a surface before calling osd_init()!");
      
  assert((draw_type > OSDDrawCall::DrawTypeInvalid && draw_type < OSDDrawCall::NumDrawTypes) &&
      "the given 'draw_type' is invalid!");

  assert((s_surface_programs[draw_type] && s_surface_programs[draw_type]->linked()) &&
      "attempted to render a surface without calling osd_init()!");

  // Return a GLProgram* or 'nullptr' if the drawcall is a no-op/invalid
  return *s_surface_programs[draw_type];
}

void OSDSurface::initGLObjects()
{
  initCommonGLObjects();

  // If created with a font - initialize related objects
  if(font_) initFontGLObjects();
}

void OSDSurface::initCommonGLObjects()
{
  GLVertexFormat empty_vertex_format;
  empty_vertex_array_ = empty_vertex_format.newVertexArray();

  empty_vertex_array_->label("a.OSD.Objects");

  surface_object_verts_ = nullptr;
  surface_object_inds_ = new GLIndexBuffer();

  // Generate the indices for drawing quads which follow the pattern:
  //    0 1 2 3 0xFFFF 4 5 6 7 0xFFFF 8 9 10 11 0xFFFF...
  std::vector<u16> inds(SurfaceIndexBufSize / sizeof(u16));
  for(u16 i = 0; i < inds.size(); i++) {
    u16 idx = (i % 5) < 4 ? (i % 5)+(i/5)*4 : 0xFFFF;

    inds[i] = idx;
  }

  surface_object_inds_
    ->alloc(SurfaceIndexBufSize, GLBuffer::StaticRead, inds.data());

  surface_object_inds_->label("bi.OSD.Objects");

  m_projection = osd_ortho(0.0f, 0.0f, (float)dimensions_.y, (float)dimensions_.x, 0.0f, 1.0f);
}

void OSDSurface::initFontGLObjects()
{
  assert(font_);
  font_tex_ = new GLTexture2D(); font_sampler_ = new GLSampler();
  strings_buf_ = new GLBufferTexture(); strings_tex_ = new GLTextureBuffer();
  string_attrs_buf_ = new GLBufferTexture(); string_attrs_tex_ = new GLTextureBuffer();

  auto& font_tex = *font_tex_;
  auto& font_sampler = *font_sampler_;

  auto num_glyphs = font_->numGlyphs();
  auto glyph_dims = font_->glyphDimensions();
  auto glyph_grid_dimensions = font_->glyphGridLayoutDimensions();

  auto tex_dimensions = ivec2 {
    glyph_grid_dimensions.x * glyph_dims.x,
    glyph_grid_dimensions.y * glyph_dims.y,
  };

  font_tex
    .alloc(tex_dimensions.x, tex_dimensions.y, 1, r8)
    .upload(0, r, GLType::u8, font_->pixelData());

  font_sampler
    .iParam(GLSampler::WrapS, GLSampler::Repeat)       // GLSampler::Repeat is crucial here because
    .iParam(GLSampler::WrapT, GLSampler::Repeat)       //   negative coordinates are used in the shader
                                                       //   to flip the font texture
    .iParam(GLSampler::MinFilter, GLSampler::Nearset)
    .iParam(GLSampler::MagFilter, GLSampler::Nearset);

  strings_buf_->alloc(StringsGPUBufSize, GLBuffer::StreamRead, GLBuffer::MapWrite);
  strings_tex_->buffer(r8ui, *strings_buf_);

  string_attrs_buf_->alloc(StringAttrsGPUBufSize, GLBuffer::StreamRead, GLBuffer::MapWrite);
  string_attrs_tex_->buffer(rgba16i, *string_attrs_buf_);

  // The projection matrix is constant for a given OSDSurface
  renderProgram(OSDDrawCall::DrawString)
    .uniformMat4x4("um4Projection", m_projection.data());

  font_tex_->label("t2d.OSD.Font");
  font_sampler_->label("s.OSD.Font");

  strings_buf_->label("bt.OSD.Strings");
  strings_tex_->label("tb.OSD.Strings");

  string_attrs_buf_->label("bt.OSD.StringAttrs");
  string_attrs_tex_->label("tb.OSD.StringAttrs");
}

void OSDSurface::destroyGLObjects()
{
  destroyCommonGLObjects();

  if(font_) destroyFontGLObjects();
}

void OSDSurface::destroyCommonGLObjects()
{
  empty_vertex_array_.destroy();

  delete surface_object_verts_;
  delete surface_object_inds_;
}

void OSDSurface::destroyFontGLObjects()
{
  delete font_tex_;
  delete font_sampler_;

  delete strings_tex_;
  delete strings_buf_;

  delete string_attrs_tex_;
  delete string_attrs_buf_;
}

// Struct intended for direct memcpy() into an
//   OpenGL buffer which stores string attributes
struct StringInstanceTexBufferData {
  u16 x, y;
  u16 offset;
  u16 size;

  u16 r, g, b, pad0;
};
static_assert(sizeof(StringInstanceTexBufferData) == (8*sizeof(u16)),
    "StringInstanceData has incorrect layout!");

void OSDSurface::appendStringDrawcalls(std::vector<OSDDrawCall>& drawcalls)
{
  // First - sort all the strings by length so they
  //   can be split into buckets which will then be used
  //   to generate drawcalls
  std::sort(string_objects_.begin(), string_objects_.end(),
      [](const auto& strobj1, const auto& strobj2) { return strobj1.str.size() < strobj2.str.size(); });

  // After sorting:
  //   back().str  -> will always be the LONGEST
  //   front().str -> will always be the SHORTEST
  const size_t size_amplitude = string_objects_.back().str.size() - string_objects_.front().str.size();

  // Number of draw calls the string drawing will be split into
  size_t num_buckets = 0;

  // Take the log base 2 of the size_amplitude as
  //   the number of buckets
  if(size_amplitude <= 1) {
    num_buckets = 1;
  } else {
    size_t tmp = size_amplitude;
    while(tmp >>= 1) num_buckets++;
  }

  const size_t strs_per_bucket = ceilf((float)string_objects_.size() / (float)num_buckets);

  auto strings_buf_mapping = strings_buf_->map(GLBuffer::MapWrite);
  auto strings_buf_ptr = strings_buf_mapping.get<u8>();
  intptr_t strings_buf_offset  = 0;

  auto string_attrs_mapping = string_attrs_buf_->map(GLBuffer::MapWrite);
  auto string_attrs_ptr = string_attrs_mapping.get<u8>();
  intptr_t string_attrs_offset = 0;

  for(size_t bucket = 0; bucket < num_buckets; bucket++) {
    // Since the bucket size is rounded UP during calculation
    //   the last bucket could contain less strings than the rest -
    //   so make sure to account from that
    size_t strs_in_bucket = (bucket+1 == num_buckets) ?
        (string_objects_.size() - strs_per_bucket*(num_buckets-1))
      : strs_per_bucket;

    // Early-out if we've no longer got anything to draw...
    if(!strs_in_bucket) break;

    auto bucket_start = string_objects_.data() + bucket*strs_per_bucket;
    auto bucket_end   = bucket_start + strs_in_bucket-1;

    const size_t bucket_str_size = bucket_end->str.size();

    for(size_t bucket_idx = 0; bucket_idx < strs_in_bucket; bucket_idx++) {
      const auto& bucket_str = *(bucket_start + bucket_idx);
      auto size = bucket_str.str.size();

      u16 color_r = bucket_str.color.r(),
          color_g = bucket_str.color.g(),
          color_b = bucket_str.color.b();

      StringInstanceTexBufferData instance_data = {
          // StringAttributes.position
          (u16)bucket_str.position.x, (u16)bucket_str.position.y,

          // StringAttributes.offset, StringAttributes.length
          (u16)strings_buf_offset, (u16)size,

          // StringAttributes.color
          color_r, color_g, color_b,

          0 /* pad0 */,
      };

      // Write the string's attributes into a buffer...
      memcpy(string_attrs_ptr + string_attrs_offset, &instance_data, sizeof(instance_data));
      string_attrs_offset += sizeof(instance_data);

      assert(string_attrs_offset < StringAttrsGPUBufSize &&
          "overflowed the string attributes gpu buffer!");

      //  ...as well as it's contents
      memcpy(strings_buf_ptr + strings_buf_offset, bucket_str.str.data(), size);
      strings_buf_offset += size;

      assert(strings_buf_offset < StringsGPUBufSize &&
          "overflowed the gpu string data buffer!");
    }

    // Append a draw-call for each bucket of strings, where:
    //   - The number of strings in this bucket (the last one could be smaller)
    //      is the instance count
    //   - The offset of the string in 'string_objects_'*2 (each string's attributes
    //       take 2 texels) is the base instance
    //   - The rest of the arguemnts are constant for every bucket's draw call,
    //      which wastes some memory, but not enough to be of immediate concern
    drawcalls.push_back(
        osd_drawcall_strings(
          empty_vertex_array_.get(), GLType::u16, surface_object_inds_, bucket*strs_per_bucket * 2,
          bucket_str_size, strs_in_bucket,
          font_tex_, font_sampler_, strings_tex_, string_attrs_tex_)
    );
  }
}

}
