#include <gx/texture.h>
#include <gx/buffer.h>
#include <gx/context.h>
#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <algorithm>
#include <utility>

#include <cassert>

namespace brdrive {

[[using gnu: always_inline]]
static constexpr auto bind_target_to_Dimensions(GLEnum bind_target) -> GLTexture::Dimensions
{
  switch(bind_target) {
  case GL_TEXTURE_1D:
  case GL_TEXTURE_BUFFER:
    return GLTexture::TexImage1D;

  case GL_TEXTURE_1D_ARRAY:
  case GL_TEXTURE_2D:
#if 0
// Almost sure there aren't needed but keeping them could save lots of typing ;)
  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
#endif
  case GL_TEXTURE_CUBE_MAP:
    return GLTexture::TexImage2D;

  case GL_TEXTURE_2D_ARRAY:
  case GL_TEXTURE_3D:
    return GLTexture::TexImage3D;

  default: ;         // Fallthrough (silence warnings)
  }

  return GLTexture::DimensionsInvalid;
}

[[using gnu: always_inline]]
static constexpr auto GLFormat_to_internalformat(GLFormat format) -> GLenum
{
  switch(format) {
  case r:     return GL_RED;
  case rg:    return GL_RG;
  case rgb:   return GL_RGB;
  case rgba:  return GL_RGBA;

  case r8:    return GL_R8;
  case rg8:   return GL_RG8;
  case rgb8:  return GL_RGB8;
  case rgba8: return GL_RGBA8;

  case r16f:  return GL_R16F;
  case rg16f: return GL_RG16F;

  case r32f:  return GL_R32F;
  case rg32f: return GL_RG32F;

  case r8i:   return GL_R8I;
  case r8ui:  return GL_R8UI;
  case r16i:  return GL_R16I;
  case r16ui: return GL_R16UI;

  case rg8i:   return GL_RG8I;
  case rg8ui:  return GL_RG8UI;
  case rg16i:  return GL_RG16I;
  case rg16ui: return GL_RG16UI;

  case rgb8i:   return GL_RGB8I;
  case rgb8ui:  return GL_RGB8UI;
  case rgb16i:  return GL_RGB16I;
  case rgb16ui: return GL_RGB16UI;

  case rgba8i:   return GL_RGBA8I;
  case rgba8ui:  return GL_RGBA8UI;
  case rgba16i:  return GL_RGBA16I;
  case rgba16ui: return GL_RGBA16UI;

  case srgb8:    return GL_SRGB8;
  case srgb8_a8: return GL_SRGB8_ALPHA8;

  case depth:    return GL_DEPTH_COMPONENT;
  case depth16:  return GL_DEPTH_COMPONENT16;
  case depth24:  return GL_DEPTH_COMPONENT24;
  case depth32f: return GL_DEPTH_COMPONENT32F;

  case depth_stencil:    return GL_DEPTH_STENCIL;
  case depth24_stencil8: return GL_DEPTH24_STENCIL8;

  default: ;         // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto GLFormat_to_format(GLFormat format) -> GLenum
{
  switch(format) {
  case r:    return GL_RED;
  case rg:   return GL_RG;
  case rgb:  return GL_RGB;
  case rgba: return GL_RGBA;

  case depth:         return GL_DEPTH_COMPONENT;
  case depth_stencil: return GL_DEPTH_STENCIL;

  default: ;         // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto GLType_to_type(GLType type) -> GLenum
{
  switch(type) {
  case GLType::u8:  return GL_UNSIGNED_BYTE;
  case GLType::u16: return GL_UNSIGNED_SHORT;
  case GLType::i8:  return GL_BYTE;
  case GLType::i16: return GL_SHORT;

  case GLType::u16_565:   return GL_UNSIGNED_SHORT_5_6_5;
  case GLType::u16_5551:  return GL_UNSIGNED_SHORT_5_5_5_1;
  case GLType::u16_565r:  return GL_UNSIGNED_SHORT_5_6_5_REV;
  case GLType::u16_1555r: return GL_UNSIGNED_SHORT_1_5_5_5_REV;

  case GLType::f32: return GL_FLOAT;

  case GLType::u32_24_8:      return GL_UNSIGNED_INT_24_8;
  case GLType::f32_u32_24_8r: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

  default: ;     // Fallthrough (silence warnings)
  }

  return GL_INVALID_ENUM;
}

GLTexture::GLTexture(GLEnum bind_target) :
  GLObject(GL_TEXTURE),
  dimensions_(bind_target_to_Dimensions(bind_target)),
  bind_target_(bind_target),
  width_(1), height_(1), depth_(1),
  levels_(~0u)
{
}

GLTexture::GLTexture(GLTexture&& other) :
  GLTexture(GL_INVALID_ENUM)
{
  other.swap(*this);
}

GLTexture::~GLTexture()
{
  GLTexture::doDestroy();
}

auto GLTexture::operator=(GLTexture&& other) -> GLTexture&
{
  destroy();
  other.swap(*this);

  return *this;
}

auto GLTexture::swap(GLTexture& other) -> GLTexture&
{
  other.GLObject::swap(*this);

  std::swap(dimensions_, other.dimensions_);
  std::swap(bind_target_, other.bind_target_);
  std::swap(width_, other.width_);
  std::swap(height_, other.height_);
  std::swap(depth_, other.depth_);
  std::swap(levels_, other.levels_);

  return *this;
}

auto GLTexture::dimensions() const -> Dimensions
{
  return dimensions_;
}

auto GLTexture::bindTarget() const -> GLEnum
{
  return GL_TEXTURE_2D;
}

auto GLTexture::width() const -> unsigned
{
  return width_;
}

auto GLTexture::height() const -> unsigned
{
  return height_;
}

auto GLTexture::depth() const -> unsigned
{
  return depth_;
}

auto GLTexture::doDestroy() -> GLObject&
{
  if(id_ == GLNullId) return *this;

  glDeleteTextures(1, &id_);
  id_ = GLNullId;   // So implicit operator=(GLTexture&&) works properly

  return *this;
}

GLTexture2D::GLTexture2D() :
  GLTexture(GL_TEXTURE_2D)
{
}

GLTexture2D::GLTexture2D(GLTexture2D&& other) :
  GLTexture2D()
{
  std::swap(id_, other.id_);
  std::swap(width_, other.width_);
  std::swap(height_, other.height_);
  std::swap(levels_, other.levels_);
}

auto GLTexture2D::alloc(
    unsigned width, unsigned height, unsigned levels, GLFormat internalformat
  ) -> GLTexture2D&
{
  // Use direct state access if available
  if(ARB::direct_state_access || EXT::direct_state_access) {
    glCreateTextures(GL_TEXTURE_2D, 1, &id_);

    glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  } else {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  assert(glGetError() == GL_NO_ERROR);

  // Initialize internal variables
  width_ = width; height_ = height;
  levels_ = levels;

  GLId bound_texture = GLNullId;

  auto gl_internalformat = GLFormat_to_internalformat(internalformat);
  if(gl_internalformat == GL_INVALID_ENUM) throw InvalidFormatTypeError();

  if(ARB::texture_storage) {
    glTextureStorage2D(id_, levels, gl_internalformat, width, height);
  } else {
    // Save current texture for future retrieval
    auto current_context = GLContext::current();
    assert(current_context);

    auto current_tex_unit = current_context->texImageUnit(current_context->activeTexture());
    bound_texture = current_tex_unit.boundTexture();

    // Allocate the whole mipmap chain
    for(unsigned l = 0; l < levels; l++) {
      glTexImage2D(GL_TEXTURE_2D, l, gl_internalformat, width, height,
        /* border */ 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

      width  = std::max(1u, width/2);
      height = std::max(1u, height/2);
    }
  }

  if(!ARB::direct_state_access) {   // Restore the previously bound texture
    glBindTexture(GL_TEXTURE_2D, bound_texture);
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLTexture2D::upload(
    unsigned level, GLFormat format, GLType type, const void *data
  ) -> GLTexture2D&
{
  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  GLId bound_texture = GLNullId;

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw InvalidFormatTypeError();

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glTextureSubImage2D(id_, level, 0, 0, width_, height_, gl_format, gl_type, data);
  } else {
    // Save current texture for future retrieval
    auto current_context = GLContext::current();
    assert(current_context);

    auto current_tex_unit = current_context->texImageUnit(current_context->activeTexture());
    bound_texture = current_tex_unit.boundTexture();

    glBindTexture(GL_TEXTURE_2D, id_);
    glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width_, height_, gl_format, gl_type, data);
  }

  // Check if the provided format/type combination is valid
  auto err = glGetError();
  if(err == GL_INVALID_OPERATION) throw InvalidFormatTypeError();

  // ...and make sure there were no other errors ;)
  assert(err == GL_NO_ERROR);

  if(!ARB::direct_state_access) {
    glBindTexture(GL_TEXTURE_2D, bound_texture);
  }

  return *this;
}

GLTextureBuffer::GLTextureBuffer() :
  GLTexture(GL_TEXTURE_BUFFER)
{
}

auto GLTextureBuffer::buffer(
    GLFormat internalformat_, const GLBuffer& buffer
  ) -> GLTextureBuffer&
{
  assert(buffer.id() != GLNullId &&
      "attempted to attach a null buffer to a GLBufferTexture!");

  auto internalformat = GLFormat_to_internalformat(internalformat_);
  assert(internalformat != GL_INVALID_ENUM);

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glCreateTextures(GL_TEXTURE_BUFFER, 1, &id_);

    glTextureBuffer(id_, internalformat, buffer.id());
  } else {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_BUFFER, id_);

    glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer.id());
  }

  assert(glGetError() == GL_NO_ERROR);

  // Write some internal variables
  width_ = buffer.size(); height_ = 1;
  levels_ = 1;

  return *this;
}


[[using gnu: always_inline]]
static constexpr auto GLSamplerParamName_to_pname(GLSampler::ParamName pname) -> GLEnum
{
  switch(pname) {
  case GLSampler::WrapS: return GL_TEXTURE_WRAP_S;
  case GLSampler::WrapT: return GL_TEXTURE_WRAP_T;
  case GLSampler::WrapR: return GL_TEXTURE_WRAP_R;

  case GLSampler::MinFilter: return GL_TEXTURE_MIN_FILTER;
  case GLSampler::MagFilter: return GL_TEXTURE_MAG_FILTER;

  case GLSampler::MinLOD:  return GL_TEXTURE_MIN_LOD;
  case GLSampler::MaxLOD:  return GL_TEXTURE_MAX_LOD;
  case GLSampler::LODBias: return GL_TEXTURE_LOD_BIAS;

  case GLSampler::CompareMode: return GL_TEXTURE_COMPARE_MODE;
  case GLSampler::CompareFunc: return GL_TEXTURE_COMPARE_FUNC;

  case GLSampler::SeamlessCubemap: return GL_TEXTURE_CUBE_MAP_SEAMLESS;

  case GLSampler::MaxAnisotropy: return GL_TEXTURE_MAX_ANISOTROPY;

  default: ;    // Fallthrough
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto GLSamplerSymbolicValue_to_param(GLSampler::SymbolicValue param) -> GLEnum
{
  switch(param) {
  case GLSampler::ClampEdge:   return GL_CLAMP_TO_EDGE;
  case GLSampler::ClampBorder: return GL_CLAMP_TO_BORDER;
  case GLSampler::Repeat:      return GL_REPEAT;

  case GLSampler::Nearset:              return GL_NEAREST;
  case GLSampler::Linear:               return GL_LINEAR;
  case GLSampler::BiLinear:             return GL_LINEAR_MIPMAP_NEAREST;
  case GLSampler::TriLinear:            return GL_LINEAR_MIPMAP_LINEAR; 
  case GLSampler::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
  case GLSampler::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;

  case GLSampler::None:            return GL_NONE;
  case GLSampler::CompareRefToTex: return GL_COMPARE_REF_TO_TEXTURE;

  case GLSampler::Eq:        return GL_EQUAL;
  case GLSampler::NotEq:     return GL_NOTEQUAL;
  case GLSampler::Less:      return GL_LESS;
  case GLSampler::LessEq:    return GL_LEQUAL;
  case GLSampler::Greater:   return GL_GREATER;
  case GLSampler::GreaterEq: return GL_GEQUAL;
  case GLSampler::Always:    return GL_ALWAYS;
  case GLSampler::Never:     return GL_NEVER;

  default: ;    // Fallthrough
  }

  return GL_INVALID_ENUM;
}

auto params_requires_SymbolicValue(GLSampler::ParamName param) -> bool
{
  switch(param) {
  case GLSampler::WrapS:
  case GLSampler::WrapT:
  case GLSampler::WrapR:
  case GLSampler::MinFilter:
  case GLSampler::MagFilter:
  case GLSampler::CompareMode:
  case GLSampler::CompareFunc:
    return true;
  }

  return false;
}

GLSampler::GLSampler() :
  GLObject(GL_SAMPLER)
{
}

GLSampler::GLSampler(GLSampler&& other) :
  GLSampler()
{
  other.swap(*this);
}

GLSampler::~GLSampler()
{
  GLSampler::doDestroy();
}

auto GLSampler::operator=(GLSampler&& other) -> GLSampler&
{
  destroy();
  other.swap(*this);

  return *this;
}

auto GLSampler::iParam(ParamName pname_, int value) -> GLSampler&
{
  // Ensure the sampler has been initialized
  initGLObject();
  assert(id_ != GLNullId);

  auto pname = GLSamplerParamName_to_pname(pname_);
  if(pname == GL_INVALID_ENUM) throw InvalidParamNameError();

  int param = -1;
  if(params_requires_SymbolicValue(pname_)) {
    param = GLSamplerSymbolicValue_to_param((SymbolicValue)value);
  } else {
    param = value;
  }

  glSamplerParameteri(id_, pname, param);

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLSampler::fParam(ParamName pname_, float value) -> GLSampler&
{
  assert(0 && "unimplemented!");

  return *this;
}

auto GLSampler::doDestroy() -> GLObject&
{
  if(id_ == GLNullId) return *this;

  glDeleteSamplers(1, &id_);
  id_ = GLNullId;   // So implicit operator=(GLSampler&&) works properly

  return *this;
}

void GLSampler::initGLObject()
{
  if(id_ != GLNullId) return;

  glGenSamplers(1, &id_);
}

GLTexImageUnit::GLTexImageUnit(GLContext *context, unsigned slot) :
  context_(context),
  slot_(slot),
  bound_texture_(GLNullId), bound_sampler_(GLNullId)
{
}

auto GLTexImageUnit::bind(const GLTexture& tex) -> GLTexImageUnit&
{
  assert(tex.id() != GLNullId && "attempted to bind() a null texture to a GLTexImageUnit!");

  auto tex_id = tex.id();

  // Only bind the texture if it's different than the current one
  if(bound_texture_ == tex_id) return *this;

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glBindTextureUnit(slot_, tex_id);
  } else {
    glActiveTexture(GL_TEXTURE0 + slot_);
    assert(glGetError() == GL_NO_ERROR);

    context_->active_texture_ = slot_;

    glBindTexture(tex.bindTarget(), tex_id);
  }
  assert(glGetError() == GL_NO_ERROR);

  bound_texture_ = tex_id;

  return *this;
}

auto GLTexImageUnit::bind(const GLSampler& sampler) -> GLTexImageUnit&
{
  assert(sampler.id() != GLNullId &&
      "attempted to bind() a null sampler to a GLTexImageUnit!");

  auto sampler_id = sampler.id();

  // Only bind the sampler if it's different than the current one
  if(bound_sampler_ == sampler_id) return *this;

  glBindSampler(slot_, sampler.id());
  bound_sampler_ = sampler.id();

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLTexImageUnit::bind(const GLTexture& tex, const GLSampler& sampler) -> GLTexImageUnit&
{
  return bind(tex), bind(sampler);
}

auto GLTexImageUnit::texImageUnitIndex() const -> unsigned
{
  return slot_;
}

auto GLTexImageUnit::boundTexture() const -> GLId
{
  return bound_texture_;
}

}
