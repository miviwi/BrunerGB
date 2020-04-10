#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/texture.h>
#include <gx/context.h>
#include <gx/extensions.h>

// OpenGL/gl3w
#include <GL/gl3w.h>

#include <cassert>

#include <new>

namespace brdrive {

[[using gnu: always_inline]]
static constexpr auto GLBufferUsage_to_usage(GLBuffer::Usage usage) -> GLEnum
{
  switch(usage) {
  case GLBuffer::StaticDraw:  return GL_STATIC_DRAW;
  case GLBuffer::DynamicDraw: return GL_DYNAMIC_DRAW;
  case GLBuffer::StreamDraw:  return GL_STREAM_DRAW;

  case GLBuffer::StaticCopy:  return GL_STATIC_COPY;
  case GLBuffer::DynamicCopy: return GL_DYNAMIC_COPY;
  case GLBuffer::StreamCopy:  return GL_STREAM_COPY;

  case GLBuffer::StaticRead:  return GL_STATIC_READ;
  case GLBuffer::DynamicRead: return GL_DYNAMIC_READ;
  case GLBuffer::StreamRead:  return GL_STREAM_READ;
  }

  return GL_INVALID_ENUM;
}

[[using gnu: always_inline]]
static constexpr auto GLBufferUsage_is_static(GLBuffer::Usage usage) -> bool
{
  auto frequency = (usage & GLBuffer::FrequencyMask) >> GLBuffer::FrequencyShift;
  return frequency == GLBuffer::Static;
}

// TODO: try to optimize this somehow..?
[[using gnu: always_inline]]
static constexpr auto GLBufferMapFlags_to_GLbitfield(u32 flags) -> GLbitfield
{
  GLbitfield access = 0;

  if(flags & GLBuffer::MapRead)  access |= GL_MAP_READ_BIT;
  if(flags & GLBuffer::MapWrite) access |= GL_MAP_WRITE_BIT;

  if(flags & GLBuffer::MapInvalidateRange)  access |= GL_MAP_INVALIDATE_RANGE_BIT;
  if(flags & GLBuffer::MapInvalidateBuffer) access |= GL_MAP_INVALIDATE_BUFFER_BIT;
  if(flags & GLBuffer::MapFlushExplicit)    access |= GL_MAP_FLUSH_EXPLICIT_BIT;
  if(flags & GLBuffer::MapUnsynchronized)   access |= GL_MAP_UNSYNCHRONIZED_BIT;
  if(flags & GLBuffer::MapPersistent)       access |= GL_MAP_PERSISTENT_BIT;
  if(flags & GLBuffer::MapCoherent)         access |= GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT;
  // Spec requires MapPersistent to ALWAYS be present with MapCoherent    ^^^^^^^^^^^^^^^^^^^^^

  if(flags & GLBuffer::DynamicStorage) access |= GL_DYNAMIC_STORAGE_BIT;
  if(flags & GLBuffer::ClientStorage)  access |= GL_CLIENT_STORAGE_BIT;

  return access;
}

GLBuffer::GLBuffer(GLEnum bind_target) :
  GLObject(GL_BUFFER),
  bind_target_(bind_target),
  size_(~0), usage_(UsageInvalid), flags_(0),
  map_counter_(0),
  mapping_(std::nullopt)
{
}

GLBuffer::GLBuffer(GLBuffer&& other) :
  GLBuffer(GL_INVALID_ENUM)
{
  other.swap(*this);
}

GLBuffer::~GLBuffer()
{
  GLBuffer::doDestroy();
}

auto GLBuffer::operator=(GLBuffer&& other) -> GLBuffer&
{
  destroy();
  other.swap(*this);

  return *this;
}

auto GLBuffer::swap(GLBuffer& other) -> GLBuffer&
{
  other.GLObject::swap(*this);

  std::swap(bind_target_, other.bind_target_);
  std::swap(size_, other.size_);
  std::swap(usage_, other.usage_);
  std::swap(flags_, other.flags_);
  std::swap(map_counter_, other.map_counter_);
  std::swap(mapping_, other.mapping_);

  return *this;
}

auto GLBuffer::alloc(GLSize size, Usage usage, u32 flags, const void *data) -> GLBuffer&
{
  assert(size > 0 && "attempted to alloc() a buffer with size <= 0");

  GLbitfield storage_flags = GLBufferMapFlags_to_GLbitfield(flags);

  if(storage_flags & (MapInvalidateRange|MapInvalidateBuffer|MapFlushExplicit|MapUnsynchronized))
    throw InvalidAllocFlagsError();

  auto is_static = GLBufferUsage_is_static(usage);
  if(is_static) {
    if(!data) throw NoDataForStaticBufferError();

    // Make sure the buffer's data is immutable by the CPU
    //   (regardless if the user requested it or not)
    //   when 'usage' matches Static*
    storage_flags &= ~GL_DYNAMIC_STORAGE_BIT;
  }

  if(ARB::buffer_storage && !is_static) {
    auto rw_flags = flags & (MapRead|MapWrite);          // MapPersistent requires at least one
    if(!rw_flags) {                                      //   of { MapRead, MapWrite } to be set
      storage_flags |= GL_MAP_READ_BIT|GL_MAP_WRITE_BIT; //   for the buffer. In case neither
    }                                                    //   one is present in 'flags' - add both,
                                                         //   which should (?) be unnoticable

    storage_flags |= GL_MAP_PERSISTENT_BIT; // See comment above GLBuffer::CachedMapping
    storage_flags |= GL_MAP_COHERENT_BIT;
  }

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glCreateBuffers(1, &id_);

    if(ARB::buffer_storage) {
      glNamedBufferStorage(id_, size, data, storage_flags);
    } else {
      glNamedBufferData(id_, size, data, GLBufferUsage_to_usage(usage));
    }
  } else {
    glGenBuffers(1, &id_);
    bindSelf();   // glBindBuffer(...)

    if(ARB::buffer_storage) {
      glBufferStorage(bind_target_, size, data, storage_flags);
    } else {
      glBufferData(bind_target_, size, data, GLBufferUsage_to_usage(usage));
    }

    unbindSelf();
  }

  assert(glGetError() == GL_NO_ERROR);

  // Initialize internal variables
  size_ = size; usage_ = usage; flags_ = storage_flags;

  return *this;
}

auto GLBuffer::alloc(GLSize size, Usage usage, const void *data) -> GLBuffer&
{
  return alloc(size, usage, 0, data);
}

auto GLBuffer::upload(const void *data) -> GLBuffer&
{
  assert(id_ != GLNullId && "attempted to upload() to a null buffer!");

  // Make sure the buffer was created with proper usage
  if(GLBufferUsage_is_static(usage_)) throw UploadToStaticBufferError();

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glNamedBufferSubData(id_, 0, size_, data);
  } else {
    bindSelf();   // glBindBuffer(...)
    glBufferSubData(bind_target_, 0, size_, data);
    unbindSelf();
  }

  assert(glGetError() == GL_NO_ERROR);

  return *this;
}

auto GLBuffer::bind() -> GLBuffer&
{
  bindSelf();

  return *this;
}

auto GLBuffer::unbind() -> GLBuffer&
{
  unbindSelf();

  return *this;
}

auto GLBuffer::map(u32 flags, intptr_t offset, GLSizePtr size) -> GLBufferMapping
{
  assert(id_ != GLNullId && "attempted to map a null buffer!");

  assert(!(offset < 0 || size < 0) && "negative offset/size passed to map()");

  // Some validation of the arguments

  if(!(flags & (MapRead|MapWrite))) throw InvalidMapFlagsError();

  if(offset >= size_) throw OffsetExceedesSizeError();
  if((offset+(size ? size : size_)) > size_) throw SizeExceedesBuffersSizeError();

  auto is_static = GLBufferUsage_is_static(usage_);
  if(is_static && map_counter_) throw RewritingStaticBufferError();

  auto proper_size = size ? size : size_;

  if(ARB::buffer_storage && !is_static) {
    // Make sure to add MapCoherent NOW
    //   i.e. before the flags compatibility
    //   check is done
    flags |= MapCoherent;

    bool cached_mapping_compatible = false;

    // Check if a CachedMapping is available...
    if(mapping_) {
      auto& mapping = mapping_.value();

      intptr_t required_size = proper_size+offset - mapping.offset;

      // Check if it's compatible with the requested params
      //   - A 'required_size' < 0 signifies the requested range
      //     comes earlier in the buffer compared to the currently
      //     mapped one
      if(required_size > 0 && mapping.size >= required_size && mapping.offset <= offset) {
        // Test if flags contains Map[Read,Write] and give a 'true'
        //   result if the current mapping has what's required - or more
        bool read_matched = (mapping.flags & MapRead) >= (flags & MapRead);
        bool write_matched = (mapping.flags & MapWrite) >= (flags & MapWrite);

        // Test if the current mapping's flags and requested ones (apart from
        //   MapRead and MapWrite) are the same
        bool rest_matched = (mapping.flags & ~(MapRead|MapWrite)) == (flags & ~(MapRead|MapWrite));

        cached_mapping_compatible = read_matched && write_matched && rest_matched;
      }

      if(cached_mapping_compatible) {
        auto new_offset = offset - mapping_->offset;
        auto new_ptr = (u8 *)mapping_->ptr + new_offset;

        map_counter_++;

        return GLBufferMapping(*this, mapping_->flags, new_ptr, new_offset, size);
      } else {
        // ...if not - destroy it and map the buffer once more
        doUnmap(MappingFriendKey(), /* force */ true);
      }
    }
  }

  auto access = GLBufferMapFlags_to_GLbitfield(flags);

  void *ptr = nullptr;
  if(ARB::direct_state_access || EXT::direct_state_access) {
    if(!offset && !size) {
      ptr = glMapNamedBufferRange(id_, 0, size_, access);
    } else if(!size) {
      size = size_ - offset;

      glMapNamedBufferRange(id_, offset, size, access);
    } else {
      glMapNamedBufferRange(id_, offset, size, access);
    }
  } else {
    bindSelf();

    if(!offset && !size) {
      ptr = glMapBufferRange(bind_target_, 0, size_, access);
    } else if(!size) {
      size = size_ - offset;

      glMapBufferRange(bind_target_, offset, size, access);
    } else {
      glMapBufferRange(bind_target_, offset, size, access);
    }

    unbindSelf();
  }

  if(!ptr || (glGetError() != GL_NO_ERROR)) throw MapFailedError();

  // Keep the data for later (for altering <un>map()'s behaviour)
  mapping_ = CachedMapping {
    ptr,
    flags,
    offset, proper_size,
  };

  map_counter_++;

  return GLBufferMapping(*this, flags, ptr, offset, size);
}

auto GLBuffer::unmap() -> GLBuffer&
{
  return doUnmap(MappingFriendKey());
}

auto GLBuffer::doUnmap(MappingFriendKey, bool force) -> GLBuffer&
{
  // Check if the buffer hasn't been previously unmap()'ped
  if(!mapping_) return *this;
  
  // After the line before we can assume mapping.has_value() == true
  auto& mapping = mapping_.value();

  // When 'force' is set to true the mapping re-use mechanism
  //   gets bypassed. This is necessary when the parent buffer
  //   has to be destroyed or the mapping recreated
  if(!force && (mapping.flags & MapCoherent)) return *this;

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glUnmapNamedBuffer(id_);
  } else {
    bindSelf();
    glUnmapBuffer(bind_target_);
    unbindSelf();
  }
  assert(glGetError() == GL_NO_ERROR);

  // ...and clear it's related data stored
  //    in the parent (i.e. this) GLBuffer
  mapping_ = std::nullopt;

  return *this;
}

void GLBuffer::doFlushMapping(MappingFriendKey, intptr_t offset, GLSizePtr length, void *ptr)
{
  assert((mapping_.has_value() && mapping_->ptr) && "attempted to flush() a null GLBufferMapping!");

  assert(!(offset < 0 || length < 0) && "offset/length passed to flush() negative!");

  // Callers of this method MUST ensure that mapping_.has_value() == true
  auto& mapping = mapping_.value();

  // Use 'ptr' to correct the offset, as the mapping
  //   could've been reused - which in turn
  //   makes it possible that the 'real' mapping
  //   starts at an earlier point in the buffer
  intptr_t delta = (u8 *)ptr - (u8 *)mapping.ptr;    // The current GLBufferMapping's size defficit
  assert(delta >= 0);

  // Advance the offset by the size defficit
  offset += delta; 

  if(!(mapping.flags & GLBuffer::MapFlushExplicit)) throw GLBufferMapping::MappingNotFlushableError();
  if(offset >= size() || (offset+length) >= size()) throw GLBufferMapping::FlushRangeError();

  if(ARB::buffer_storage) {
    auto mapping_coherent = mapping.flags & GLBuffer::MapCoherent;
    if(mapping_coherent) return;
  }

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glFlushMappedNamedBufferRange(id(), offset, length);
  } else {
    glBindBuffer(bindTarget(), id());
    glFlushMappedBufferRange(bindTarget(), offset, length);
    glBindBuffer(bindTarget(), 0);    // unbind
  }
}

auto GLBuffer::bindTarget() const -> GLEnum
{
  return bind_target_;
}

auto GLBuffer::size() const -> GLSize
{
  return size_;
}

void GLBuffer::bindSelf()
{
  assert(id_ != GLNullId && "attempted to use a null buffer!");

  glBindBuffer(bind_target_, id_);
}

void GLBuffer::unbindSelf()
{
  glBindBuffer(bind_target_, 0);
}

auto GLBuffer::doDestroy() -> GLBuffer&
{
  if(id_ == GLNullId) return *this;

  // First - unmap the buffer if it's still mapped
  if(mapping_) doUnmap(MappingFriendKey(), /* force */ true);

  glDeleteBuffers(1, &id_);

  return *this;
}

GLBufferMapping::GLBufferMapping(
    GLBuffer& buffer, u32 /* Flags */ flags, void *ptr,
    intptr_t offset, GLSizePtr size
) :
  buffer_(buffer), flags_(flags), ptr_(ptr),
  offset_(offset), size_(size)
{
  assert(ptr_ &&
      "initialized a GLBufferMapping with nullptr! (maybe the glMapBuffer() call failed?)");
}

GLBufferMapping::~GLBufferMapping()
{
  buffer_.unmap();
}

auto GLBufferMapping::get() -> void *
{
  return ptr_;
}

auto GLBufferMapping::get() const -> const void *
{
  return ptr_;
}

auto GLBufferMapping::flush(intptr_t offset, GLSizePtr length) -> GLBufferMapping&
{
  if(!ptr_) throw FlushUnmappedError();

  buffer_.doFlushMapping(GLBuffer::MappingFriendKey(), offset, length, ptr_);
  return *this;
}

void GLBufferMapping::unmap()
{
  assert(ptr_ && "attempted to unmap() a null mapping!");

  buffer_.doUnmap(GLBuffer::MappingFriendKey());
  ptr_ = nullptr;     // Mark the mapping object itself as unmapped
}

GLVertexBuffer::GLVertexBuffer() :
  GLBuffer(GL_ARRAY_BUFFER)
{
}

GLIndexBuffer::GLIndexBuffer() :
  GLBuffer(GL_ELEMENT_ARRAY_BUFFER)
{
}

GLUniformBuffer::GLUniformBuffer() :
  GLBuffer(GL_UNIFORM_BUFFER)
{
}

[[using gnu: always_inline]]
static constexpr auto XferDirection_to_bind_target(
    GLPixelBuffer::XferDirection xfer_direction
  ) -> GLEnum
{
  switch(xfer_direction) {
  case GLPixelBuffer::Upload:   return GL_PIXEL_UNPACK_BUFFER;
  case GLPixelBuffer::Download: return GL_PIXEL_PACK_BUFFER;

  default: assert(0);     // Unreachable
  }

  return GL_INVALID_ENUM;
}

// TODO: merge these functions with the ones in gx/texture.cpp (?)
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

// TODO: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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

GLPixelBuffer::GLPixelBuffer(XferDirection xfer_direction) :
  GLBuffer(XferDirection_to_bind_target(xfer_direction)),
  xfer_direction_(xfer_direction)
{
}

// Extract the currently bound texture's name at the
//   bind target 'bind_target'
//
// TODO: somehow restore the name of the texture bound to
//       the current image unit without querying OpenGL's state...
static auto get_currently_bound_tex(GLEnum bind_target) -> GLEnum
{
  // Have to convert between the bind target enum's value
  //   which has the for GL_TEXTURE_* to one of the form
  //   GL_TEXTURE_BINDING_* which is siutable for glGet
  auto pname = ([bind_target]() -> GLEnum {
    switch(bind_target) {
    case GL_TEXTURE_1D:       return GL_TEXTURE_BINDING_1D;
    case GL_TEXTURE_1D_ARRAY: return GL_TEXTURE_BINDING_1D_ARRAY;

    case GL_TEXTURE_2D:       return GL_TEXTURE_BINDING_2D;
    case GL_TEXTURE_2D_ARRAY: return GL_TEXTURE_BINDING_2D_ARRAY;

    case GL_TEXTURE_CUBE_MAP: return GL_TEXTURE_BINDING_CUBE_MAP;

    case GL_TEXTURE_3D: return GL_TEXTURE_BINDING_3D;

    default: ;    // Fallthrough (silence warnings)
    }

    return GL_INVALID_ENUM;
  })();
  assert(pname != GL_INVALID_ENUM);

  int current_tex = -1;

  glGetIntegerv(pname, &current_tex);
  assert(glGetError() == GL_NO_ERROR);

  return current_tex;
}

auto GLPixelBuffer::uploadTexture(
    GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset_
  ) -> GLPixelBuffer&
{
  assert(id_ != GLNullId && "attempted to uploadTexture() from a null GLPixelBuffer!");
  
  // Make sure this buffer is a GLPixelBuffer(Upload)
  static constexpr XferDirection direction = Upload;
  if(xfer_direction_ != direction) throw InvalidXferDirectionError<direction>();

  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw GLTexture::InvalidFormatTypeError();

  // Convert the offset to a void* as glTextureImage* functions
  //   are overloaded as such when uploading from a buffer
  auto offset = (void *)offset_;

  // Have to bind the buffer to the context
  //   whether DSA is available or not
  bindSelf();

  if(ARB::direct_state_access || EXT::direct_state_access) {
    switch(tex.dimensions()) {
    case GLTexture::TexImage2D:
      glTextureSubImage2D(
          tex.id(), level, 0, 0, tex.width(), tex.height(),
          gl_format, gl_type, offset /* The data source is a GLPixelBuffer(Upload) */
      );
      break;

    default: assert(0 && "TODO: unimplemented!"); break;
    }
  } else {
    auto bind_target = tex.bindTarget();

    // Save the currently bound texture...
    auto current_tex = get_currently_bound_tex(bind_target);

    // ...bind the target texture...
    glBindTexture(bind_target, tex.id());

    // ..upload the buffer's data to it...
    switch(tex.dimensions()) {
    case GLTexture::TexImage2D:
      glTexSubImage2D(
          bind_target, level, 0, 0, tex.width(), tex.height(),
          gl_format, gl_type, offset /* The data source is a GLPixelBuffer(Upload) */
      );
      break;

    default: assert(0 && "TODO: unimplemented!"); break;
    }

    // ... and restore the previously bound one
    glBindTexture(bind_target, current_tex);
  }

  assert(glGetError() == GL_NO_ERROR);

  // Make sure that future texture operations don't interact with the buffer
  unbindSelf();

  return *this;
}

auto GLPixelBuffer::downloadTexture(
    const GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset_
  ) -> GLPixelBuffer&
{
  assert(id_ != GLNullId && "attempted to downloadTexture() to a null GLPixelBuffer!");

  // Make sure this buffer is a GLPixelBuffer(Download)
  static constexpr XferDirection direction = Download;
  if(xfer_direction_ != direction) throw InvalidXferDirectionError<direction>();

  auto gl_format = GLFormat_to_format(format);
  auto gl_type   = GLType_to_type(type);

  if(gl_format == GL_INVALID_ENUM || gl_type == GL_INVALID_ENUM)
    throw GLTexture::InvalidFormatTypeError();

  // Convert the offset to a void* as glGetTextureImage() function
  //   is overloaded as such when downloading to a buffer
  auto offset = (void *)offset_;

  // Have to bind the buffer to the context
  //   whether DSA is available or not
  bindSelf();

  if(ARB::direct_state_access || EXT::direct_state_access) {
    glGetTextureImage(
        tex.id(), level, gl_format, gl_type, size_,
        offset /* The data destination buffer is a GLPixelBuffer(Download) */
    );
  } else {
    auto bind_target = tex.bindTarget();

    // Save the currently bound texture to restore it after the
    //  glGetTexImage() call is made
    auto current_tex = get_currently_bound_tex(bind_target);
    
    // Bind the source texture for the download...
    glBindTexture(bind_target, tex.id());

    // ...actually perform the download...
    //  - Use the safer (in this context) version of glGetTexImage()
    glGetnTexImage(
        bind_target, level, gl_format, gl_type, size_,
        offset /* The data destination buffer is a GLPixelBuffer(Download) */
    );

    assert(glGetError() == GL_NO_ERROR);

    // ...and restore the previously bound texture
    glBindTexture(bind_target, current_tex);
  }

  // Make sure that future texture operations don't interact with the buffer
  unbindSelf();

  return *this;
}

GLBufferTexture::GLBufferTexture() :
  GLBuffer(GL_TEXTURE_BUFFER)
{
}

[[using gnu: always_inline]]
static constexpr auto GLBufferBindPointType_to_target(GLBufferBindPointType type) -> GLEnum
{
  switch(type) {
  case UniformType:       return GL_UNIFORM_BUFFER;
  case ShaderStorageType: return GL_SHADER_STORAGE_BUFFER;
  case XformFeedbackType: return GL_TRANSFORM_FEEDBACK_BUFFER;

  default: assert(0);    // It's the object owner's responsibility to make sure 'type' is always valid
  }

  return GL_INVALID_ENUM;   // Unreachable
}

GLBufferBindPoint::GLBufferBindPoint(
    GLContext *context, GLBufferBindPointType type, unsigned index
) :
  context_(context),
  target_(GLBufferBindPointType_to_target(type)), index_(index),
  bound_buffer_(GLNullId)
{
}

auto GLBufferBindPoint::bind(
    const GLBuffer& buffer, intptr_t offset, GLSizePtr size
  ) -> GLBufferBindPoint&
{
  assert(!(offset < 0 || size < 0) && "offset/size negative passed to GLBufferBindPoint::bind()!");
  assert(buffer.id() != GLNullId && "attemmpted to bind() a null buffer!");
  assert(buffer.bindTarget() == target_ &&
      "attempted to bind() a buffer with an incompatible bindTarget() to a bind point!");

  GLId bufferid = buffer.id();
  GLSize buffer_size = buffer.size();

  // Make sure all the arguments are valid...
  if(offset >= buffer_size) throw GLBuffer::OffsetExceedesSizeError();
  if((offset+size) >= buffer_size) throw GLBuffer::SizeExceedesBuffersSizeError();

  // Only bind the buffer if it's different than
  //   the one currently bound to this bind point
  if(bound_buffer_ == bufferid) return *this;

  if(!size && !offset) {
    // If neither the offset nor the size has been specified glBindBufferBase() can be used
    glBindBufferBase(target_, index_, bufferid);
  } else {
    // Check if we need to compute the left-over space
    if(!size) size = buffer_size - offset;

    glBindBufferRange(target_, index_, bufferid, offset, size);
  }
  bound_buffer_ = bufferid;

  return *this;
}
  

}
