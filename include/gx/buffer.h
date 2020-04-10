#pragma once

#include <gx/gx.h>
#include <gx/object.h>

#include <exception>
#include <stdexcept>
#include <optional>

namespace brdrive {

// Forward declarations
class GLContext;
class GLBufferMapping;
class GLVertexArray;
class GLTexture;

class GLBuffer : public GLObject {
public:
  // The values are created such that
  //     Usage: 0b0000'ffaa
  //  where each digit/symbol corresponds
  //  to a single bit and
  //    - 'f' is the frequency of access
  //      { Static, Dynamic, Stream }
  //    - 'a' is the type of access
  //      { Read, Copy, Draw }
  enum Usage {
    Static = 0, Dynamic = 1, Stream = 2,
    Read = 0, Copy = 1, Draw = 2,

    FrequencyMask  = 0b00001100,
    AccessTypeMask = 0b00000011,

    FrequencyShift  = 2,
    AccessTypeShift = 0,

    StaticRead  = (Static<<FrequencyShift)|Read,
    DynamicRead = (Dynamic<<FrequencyShift)|Read,
    StreamRead  = (Stream<<FrequencyShift)|Read,

    StaticCopy  = (Static<<FrequencyShift)|Copy,
    DynamicCopy = (Dynamic<<FrequencyShift)|Copy,
    StreamCopy  = (Stream<<FrequencyShift)|Copy,

    StaticDraw  = (Static<<FrequencyShift)|Draw,
    DynamicDraw = (Dynamic<<FrequencyShift)|Draw,
    StreamDraw  = (Stream<<FrequencyShift)|Draw,

    UsageInvalid = ~0,
  };

  enum Flags : u32 {
    // Map Flags
    MapRead  = (1<<0),
    MapWrite = (1<<1),

    MapInvalidateRange  = (1<<2),
    MapInvalidateBuffer = (1<<3),
    MapFlushExplicit    = (1<<4),
    MapUnsynchronized   = (1<<5),
    MapPersistent       = (1<<6),
    MapCoherent         = (1<<7),
    
    // Storage flags
    DynamicStorage = (1<<8),
    ClientStorage  = (1<<9),
  };

  struct InvalidAllocFlagsError : public std::runtime_error {
    InvalidAllocFlagsError() :
      std::runtime_error(
          "only { MapRead, MapWrite, MapPersistent, MapCoherent, DynamicStorage, ClientStorage }"
          " may be included in he 'flags' argument to alloc()")
      { }
  };

  struct NoDataForStaticBufferError : public std::runtime_error {
    NoDataForStaticBufferError() :
      std::runtime_error("a 'Static' GLBuffer MUST be supplied with data upon allocation!")
    { }
  };

  struct UploadToStaticBufferError : public std::runtime_error {
    UploadToStaticBufferError() :
      std::runtime_error("cannot upload() to a buffer with 'Static' usage frequency!")
    { }
  };

  struct RewritingStaticBufferError : public std::runtime_error {
    RewritingStaticBufferError() :
      std::runtime_error("cannot map() a buffer with 'Static' usage frequency more than once!")
    { }
  };

  struct InvalidBindingIndexError : public std::runtime_error {
    InvalidBindingIndexError() :
      std::runtime_error("the 'index' for an indexed bind must be in the range [0;MaxBindIndex]")
    { }
  };
  
  struct OffsetExceedesSizeError : public std::runtime_error {
    OffsetExceedesSizeError() :
      std::runtime_error("the offset specified exceedes the buffer's size!")
    { }
  };

  struct OffsetAlignmentError : public std::runtime_error {
    OffsetAlignmentError() :
      std::runtime_error("the offset MUST be aligned on a target specific boundary!"
          " (the alignment can be queried via the buffer's bindOffsetAlignment() method)")
    { }
  };

  struct SizeExceedesBuffersSizeError : public std::runtime_error {
    SizeExceedesBuffersSizeError() :
      std::runtime_error("the requested size is > the buffer's size (possibly reduced by the passed 'offset'")
    { }
  };

  struct InvalidMapFlagsError : public std::runtime_error {
    InvalidMapFlagsError() :
      std::runtime_error("the flags MUST contain at least one of { MapRead, MapWrite }")
    { }
  };

  struct MapFailedError : public std::runtime_error {
    MapFailedError() :
      std::runtime_error("the call to glMapBuffer() failed")
    { }
  };

  GLBuffer(GLBuffer&& other);
  virtual ~GLBuffer();

  auto operator=(GLBuffer&& other) -> GLBuffer&;

  // Allocate the GL object for the buffer and it's backing memory
  //   - MUST be called before any other method ex. map(), upload() etc.
  auto alloc(
      GLSize size, Usage usage, u32 /* Flags */ flags = 0, const void *data = nullptr
    ) -> GLBuffer&;

  // Same as alloc() above, except the flags parameter
  //   is ommited (defaults to 0)
  auto alloc(
      GLSize size, Usage usage, const void *data = nullptr
    ) -> GLBuffer&;

  auto upload(const void *data) -> GLBuffer&;

  auto bind() -> GLBuffer&;
  auto unbind() -> GLBuffer&;

  // Only ONE mapping of a given buffer may exist at a time!
  auto map(u32 /* Flags */ flags, intptr_t offset = 0, GLSizePtr size = 0) -> GLBufferMapping;

  // Also called in ~GLBufferMapping, so a manual
  //   call ism't needed
  auto unmap() -> GLBuffer&;

  auto bindTarget() const -> GLEnum;
  auto size() const -> GLSize;

/*
semi-private:
*/
  class MappingFriendKey {
    MappingFriendKey() { }

    friend GLBuffer;
    friend GLBufferMapping;
  };

  auto doUnmap(MappingFriendKey, bool force = false) -> GLBuffer&;

  void doFlushMapping(MappingFriendKey, intptr_t offset, GLSizePtr length, void *ptr);

protected:
  GLBuffer(GLEnum bind_target);

  auto swap(GLBuffer& other) -> GLBuffer&;

  virtual auto doDestroy() -> GLBuffer& final;

  // Binds this buffer to the context
  void bindSelf();
  // Binds 0 to bind_tagret_ (which,
  //   effectively, unbinds any buffer
  //   from said target)
  void unbindSelf(); 

  GLEnum bind_target_;

  GLSize size_;
  Usage usage_;
  u32 flags_;

  // Increments by 1 everytime map() is called
  unsigned map_counter_;

  // Stores information on the currently mapped
  //   buffer region, which is used to attempt
  //   it's reuse by way of ARB::buffer_storage's
  //   'MapPersistent' buffer flag.
  // It is implemented like so:
  //   - Each buffer keeps a std::optional<CachedMapping>
  //     which initially has the value std::nullopt
  //   - alloc() (which needs to be called to create
  //     the backing OpenGL object) checks for the
  //     presence of ARB::buffer_storage - and when
  //     found - the buffer's creation flags are
  //     extended with:
  //          MapPersistent|MapCoherent
  //   - When map() is called, the mapping flags
  //     are also extended by:
  //          MapPersistent|MapCoherent
  //     (though only if ARB::buffer_storage is
  //     available), then 'mapping_' is inspected
  //     and if it contains a value that means
  //     the mapped buffer is still available in
  //     the address space and so - the requested
  //     offset, size and flags for the new mapping
  //     are checked against those of the cached
  //     one:
  //          * The cached mapping's offset needs
  //            to come before the requested offset
  //          * The cached mapping's size must be
  //            big enough to accomodate the range
  //               [offset;offset+size] (relative to buffer)
  //          * Have the requested MapRead/MapWrite
  //            flags or more eg. a cached ReadWrite
  //            mapping can be reused as a read-only
  //            one
  //          * The rest of the flags must be ==
  //            to the requested ones
  //     A.) If all the above requiremets are fulfilled
  //         map() returns a mapping based on the cached
  //         one with it's base pointer shifted by the
  //         difference between the offsets and the
  //         offset adjusted by the offset of the cached
  //         mapping.
  //     B.) Otherwise the cached mapping is unmapped
  //         with force=true and a new one is created
  //         the same way as the first one
  //   - The unmap() operation gets spoofed by a function
  //     which ignores the request, this is possible
  //     due to the 'MapCoherent' flag, which replicates
  //     the first part of unmap()'s behaviour - the data
  //     written by the host becomes visible on the
  //     device. HOWEVER while the affected mapping
  //     object (GLBufferMapping) gets invalidated,
  //     the mapping's properties are stil kept in
  //     the parent GLBuffer - ready for possible
  //     reuse. This setup however, would create memory
  //     leaks since when the destructor gets invoked
  //     and attempts to unmap the cached mapping -
  //     it will be flushed just like any other unmap()
  //     call. For this reason there exists a method
  //     only usable by GLBuffer and GLBufferMapping
  //     doUnmap(), which takes a [bool: force] para-
  //     -meter, this parameter - when == true - causes
  //     glUnmap() to be called regardless if the
  //     caching mechanism is engaged or not (a
  //     doUnmap(force=true) call is also made inside
  //     map() when the cached and requested mapping
  //     parameters are incompatible and a new mapping
  //     is actually created)
  //   - The flush() operation needs only minor changes
  //     i.e. when using a cached mapping as a new one,
  //     because flush()'es 'offset' parameter is
  //     relative to the start of the mapped range -
  //     it has to be adjusted.
  // TODO: implement a smarter way of enabling the
  //       caching mechanism, because under certain
  //       conditions it WILL do more harm than
  //       good.
  struct CachedMapping {
    void *ptr;
    u32 flags;
    intptr_t offset;
    GLSizePtr size;
  };

  // Set to std::nullopt by default, calling
  //   map() emplaces a CachedMapping or
  //   reads what's already there. 
  // The value gets reset to std::nullopt
  //   when the mapping caching mechanism
  //   isn't used and unmap() is called,
  //   or by way of:
  //          doUnmap(force=true)
  std::optional<CachedMapping> mapping_;
};

class GLBufferMapping {
public:
  struct MappingNotFlushableError : public std::runtime_error {
    MappingNotFlushableError() :
      std::runtime_error("flush() can be used only when the buffer"
          " was mapped with the GLBuffer::MapFlushExplicit flag!")
    { }
  };

  struct FlushRangeError : public std::runtime_error {
    FlushRangeError() :
      std::runtime_error("attempted to flush the buffer past the mapped range!"
          " (either the offset > mapped_size | size > mapped_size | offset+size > mapped_size)")
    { }
  };

  struct FlushUnmappedError : public std::runtime_error {
    FlushUnmappedError() :
      std::runtime_error("cannot flush() a mapping which has previously been unmap()'ped")
    { }
  };

  GLBufferMapping(const GLBufferMapping&) = delete;
  ~GLBufferMapping();

  // Returns a pointer to the start of
  //   the mapped buffer's range
  auto get() -> void *;
  auto get() const -> const void *;

  // Helpers to eliminate need for
  //   casting void* from the
  //   non-templated get()
  template <typename T>
  auto get() -> T *
  {
    return (T*)get();
  }
  template <typename T>
  auto get() const -> const T *
  {
    return (const T *)get();
  }

  // Returns the n-th element of the
  //   mapped buffer's range as if
  //   it is an array of T
  template <typename T>
  auto at(size_t n) -> T&
  {
    return *(get<T>() + n);
  }
  template <typename T>
  auto at(size_t n) const -> const T&
  {
    return *(get<T>() + n);
  }

  // Returns 'true' if the mapping hasn't been unmapped
  operator bool() { return ptr_; }

  // Ensures data written by the host in the range [offset;offset+size]
  //   becomes visible on the device
  //  - Calling this method on a buffer/mapping created without the
  //    MapFlushExplicit flag will throw an exception!
  auto flush(intptr_t offset = 0, GLSizePtr length = 0) -> GLBufferMapping&;

  // If called more than once on a given mapping the result is a no-op
  void unmap();

private:
  friend GLBuffer;

  GLBufferMapping(
      GLBuffer& buffer, u32 /* Flags */ flags, void *ptr,
      intptr_t offset, GLSizePtr size
  );

  GLBuffer& buffer_;
  u32 /* GLBuffer::Flags */ flags_;
  void *ptr_;

  intptr_t offset_; GLSizePtr size_;
};

class GLVertexBuffer : public GLBuffer {
public:
  GLVertexBuffer();
};

// NOTE: an index buffer can be allocated (strictly speaking
//       only without DSA) ONLY while a vertex array is bound
class GLIndexBuffer : public GLBuffer {
public:
  GLIndexBuffer();
};

class GLUniformBuffer : public GLBuffer {
public:
  GLUniformBuffer();
};

class GLPixelBuffer : public GLBuffer {
public:
  enum XferDirection {
    Upload, Download,
  };

  template <XferDirection xfer_direction>
  struct InvalidXferDirectionError : public std::runtime_error {
    InvalidXferDirectionError() :
      std::runtime_error(error_message())
    { }

  private:
    static auto error_message() -> const char *
    {
      if constexpr(xfer_direction == Upload) {
        return "used a GLPixelBuffer(Download) for an upload() operation";
      } else if(xfer_direction == Download) {
        return "used a GLPixelBuffer(Upload) for a download() operation";
      }

      return nullptr;   // Unreachable
    }
  };
  
  GLPixelBuffer(XferDirection xfer_direction);

  // Upload the buffer's data to the texture
  //   - 'format' and 'type' describe the format of the BUFFER's pixels
  //   - 'offset' is a byte-offset into the buffer where the data
  //      to be uploaded resides
  //   - throws GLTexture::InvalidFormatTypeError() when the specified
  //     type or format/type combination is invalid
  auto uploadTexture(
      GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset = 0
    ) -> GLPixelBuffer&;
  // Fill the buffer with the texture's data
  //   - 'format' and 'type' describe the format of the pixels in the
  //      BUFFER after the download completes
  //   - 'offset' is a byte-offset into the buffer where the data
  //      will be written
  //   - throws GLTexture::InvalidFormatTypeError() when the specified
  //     type or format/type combination is invalid
  auto downloadTexture(
      const GLTexture& tex, unsigned level, GLFormat format, GLType type, uptr offset = 0
    ) -> GLPixelBuffer&;

private:
  XferDirection xfer_direction_;
};

class GLBufferTexture : public GLBuffer {
public:
  GLBufferTexture();
};

enum GLBufferBindPointType : unsigned {
  UniformType,
  ShaderStorageType,
  XformFeedbackType,

  NumTypes,
};

class GLBufferBindPoint {
public:
  // - When the 'size' is not specified the whole buffer
  //   will be bound to this bind point
  auto bind(
      const GLBuffer& buffer, intptr_t offset = 0, GLSizePtr size = 0
    ) -> GLBufferBindPoint&;

private:
  friend GLContext;

  GLBufferBindPoint(GLContext *context, GLBufferBindPointType type, unsigned index);

  GLContext *context_;

  GLEnum target_;
  unsigned index_;

  GLId bound_buffer_;
};

}
