#pragma once

#include <gx/gx.h>
#include <gx/object.h>

#include <exception>
#include <stdexcept>
#include <array>
#include <optional>

namespace brdrive {

// Forward declarations
class GLVertexBuffer;
class GLVertexArray;
class GLVertexArrayHandle;

struct GLVertexFormatAttr {
  enum Type : u16 {
    Normalized = 0, UnNormalized = 1,    // glVertexAttribFormat()
    Integer = (1<<1),                    // glVertexAttribIFormat()

    PerInstance = (1<<2),     // glVertexBindingDivisor(1)

    AttrInvalid = 0xFFFF,
  };

  Type attr_type = AttrInvalid;

  unsigned buffer_index; // The 'bindingindex' which will be passed to glVertexAttribBinding()
  GLSize num_components; // MUST be in the range [1;4]
  GLEnum type; // Stores the OpenGL GLenum representation of the attribute's GLType
  GLSize offset;   // The offset of this attribute's data RELATIVE to the offset
                   //   specified for the backing vertex buffer. So:
                   //       <data offset> = <GLVertexFormatBuffer.offset> + <offset>

  auto attrByteSize() const -> GLSize;

  // A helper method for creating the 'pointer'
  //   parameter for the glVertexAttrib*Pointer()
  //   family of functions
  auto offsetAsPtr() const -> const void *;

  // Returns 'true' when attr_type == Normalized
  //   and 'false' otherwise
  // NOTE: this function's result MUST be
  //       converted to a GLboolean:
  //         GLboolean: { GL_TRUE, GL_FALSE }
  //       if it's meant to be used as an argument
  //       to an OpenGL call
  auto normalized() const -> bool;
};

struct GLVertexFormatBuffer {
  GLId bufferid = GLNullId;

  GLSize stride, offset;
};

class GLVertexFormat {
public:
  // All the constants below are set to values
  //   which the spec guarantees will be >= on
  //   ANY OpenGL-compatible hardware

  static constexpr unsigned MaxVertexAttribs = 16;
  static constexpr unsigned MaxVertexBufferBindings = 16;

  static constexpr unsigned MaxVertexAttribRelativeOffset = 2047;
  // Could be interpreted as the maximum size of the vertex
  static constexpr unsigned MaxVertexAttribStride = 2048;
  static constexpr unsigned MaxVertexSize         = MaxVertexAttribStride;

  using AttrType = GLVertexFormatAttr::Type;

  struct InvalidAttribTypeError : public std::runtime_error {
    InvalidAttribTypeError() :
      std::runtime_error("an invalid GLType was passed to [i]attr()!")
    { }
  };

  struct ExceededAllowedAttribCountError : public std::runtime_error {
    ExceededAllowedAttribCountError() :
      std::runtime_error("the maximum allowed number (GLVertexFormat::MaxVertexAttribs) of"
          " attributes of a vertex format has been exceeded!")
    { }
  };

  struct VertexBufferBindingIndexOutOfRangeError : public std::runtime_error {
    VertexBufferBindingIndexOutOfRangeError() :
      std::runtime_error("the values of buffer binding point indices cannot be greater"
        " than GLVertexFormat::MaxVertexBufferBindings")
    { }
  };

  struct InvalidNumberOfComponentsError : public std::runtime_error {
    InvalidNumberOfComponentsError() :
      std::runtime_error("the number of the attribute's components ('size' argument)"
          " is not in the allowed range [1;4]")
    { }
  };

  struct VerterAttribOffsetOutOfRangeError : public std::runtime_error {
    VerterAttribOffsetOutOfRangeError() :
      std::runtime_error("the offset of the attribute exceedes the max allowed value"
        " (GLVertexFormat::MaxVertexSize)")
    { }
  };

  struct VertexExceedesMaxSizeError : public std::runtime_error {
    VertexExceedesMaxSizeError() :
      std::runtime_error("the size of the vertex (= size of attributes + padding_bytes)"
          " has exceeded the possible maximum! (GLVertexFormat::MaxVertexSize)")
    { }
  };

  struct AttribWithoutBoundVertexBufferError : public std::runtime_error {
    AttribWithoutBoundVertexBufferError() :
      std::runtime_error("not all vertex buffer binding slots referenced by vertex attributes"
          " have a vertex buffer bound!")
    { }
  };

  struct StrideExceedesMaxAllowedError : public std::runtime_error {
    StrideExceedesMaxAllowedError() :
      std::runtime_error("the 'stride' for vertex buffer attribute data CANNOT"
          " be > than GLVertexFormat::MaxVertexAttribStride!")
    { }
  };

  struct PerVertexAttribInPerInstanceBufferError : public std::runtime_error {
    PerVertexAttribInPerInstanceBufferError() :
      std::runtime_error("the vertex buffer at this index can ONLY contain 'PerInstance'"
          " vertex attributes (as one such attribute has already been assigned)!")
    { }
  };

  struct PerInstanceAttribInPerVertexBufferError : public std::runtime_error {
    PerInstanceAttribInPerVertexBufferError() :
      std::runtime_error("the vertex buffer at this index CANNOT contain 'PerInstance'"
          " vertex attributes (as a per vertex attribute has already been assigned)!")
    { }
  };

  GLVertexFormat();
//  GLVertexFormat(const GLVertexFormat&) = delete;
//  GLVertexFormat(GLVertexFormat&& other);

  // Append an attribute exposed as a single precision (32-bit)
  //   floating point number/vector -> float/vec2/vec3/vec4 to GLSL shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  //  - When 'offset' isn't passed explicitly, then - the size of
  //    all attributes added before this call (vertexByteSize() is
  //    used to compute it) is used
  auto attr(
      unsigned buffer_index, int num_components, GLType type,
      u16 attr_type = GLVertexFormatAttr::Normalized, GLSize offset = -1
    ) -> GLVertexFormat&;

  // Append an attribute exposed as a (32-bit) integer/vector
  //   of integers -> int/ievc2/ivec3/ivec4 in shaders
  //  - The attribute indices are assigned sequantially
  //    starting at 0
  //  - When 'offset' isn't passed explicitly, then - the size of
  //    all attributes added before this call (vertexByteSize() is
  //    used to compute it) is used
  auto iattr(
      unsigned buffer_index, int num_components, GLType type,
      u16 attr_type = GLVertexFormatAttr::Integer, GLSize offset = -1
    ) -> GLVertexFormat&;

  // Adds 'padding_bytes' of padding to the end of the format's
  //   vertices - i.e. makes the size of the vertex:
  //     vertex_size = combined size of attributes + padding_bytes
  auto padding(GLSize padding_bytes) -> GLVertexFormat&;

  // Returns the combined size of all of the attributes (taking
  //   into account the offsets, which could be used to pad out
  //   attributes - increasing size) which DO NOT have the
  //   'PerInstance' flag set!
  auto vertexByteSize() const -> GLSize;

  // Returns the size of all the attributes which are marked as
  //    'PerInstance'
  auto instanceByteSize() const -> GLSize;

  // 'index' corresponds to GLVertexFormatAttrib::buffer_index
  auto bindVertexBuffer(
      unsigned index, const GLVertexBuffer& vertex_buffer,
      GLSize stride = -1, GLSize offset = 0
    ) -> GLVertexFormat&;

  // Create a new vertex array according to all
  //   recorded attributes and their formats
  //  - All the vertex buffer binding points referenced by
  //    attributes MUST have buffers bound before calling
  //    this method! (strictly speaking - only because
  //    the ARB_vertex_array_object code path requires it,
  //    but in the name of homogenity this is enforced
  //    as a requirement)
  //  - After the method returns the stored vertex buffer
  //    bindings (ones internal to this GLVertexFormat)
  //    are cleared, this is done to make creating multiple
  //    vertex arrays from a single GLVertexFormat instance
  //    possible
  auto createVertexArray() const -> GLVertexArray;

  // The same as createVertexArray(), except a heap
  //   pointer is returned instead of a stack object
  auto newVertexArray() const -> GLVertexArrayHandle;

  // Must be called BEFORE createVertexArray() to take effect
  void dbg_ForceVertexArrayCreatePath(int path);

private:
  using AttributeArray = std::array<GLVertexFormatAttr, MaxVertexAttribs>;
  using BufferArray    = std::array<GLVertexFormatBuffer, MaxVertexBufferBindings>;

  //     returns -> attributes_.at(current_attrib_index_)
  auto currentAttrSlot() -> GLVertexFormatAttr&;

  // If the currentAttrSlot() (with index current_attrib_index_)
  //   isn't taken the method returns 'current_attrib_index_',
  // However if it IS taken, then the slots are scanned one-by-one
  //   (by incrementing 'current_attrib_index_') and the index
  //   of the first empty slot found is kept in current_attrib_index_
  //   and returned.
  auto nextAttrSlotIndex() -> unsigned;

  // Add the desired vertex attribute at the next free index
  auto appendAttr(
      unsigned buffer_index, int num_components, GLType type, GLSize offset, int attr_type
    ) -> GLVertexFormat&;

  // Returns 'true' if any call made to [i]attr() matched
  //       attr(buf_binding_index, ...)
  //  i.e. it sets the buffer bound to 'buf_binding_index'
  //       as that attribute's data source
  auto usesVertexBuffer(unsigned buf_binding_index) -> bool;

  // createVertexArray() implementation for the ARB_vertex_attrib_binding path
  auto createVertexArray_vertex_attrib_binding() const -> GLVertexArray;
  // createVertexArray() implementation for fallback path - platforms which
  //   only support ARB_vertex_array_object
  auto createVertexArray_vertex_array_object() const -> GLVertexArray;

  void recalculateSizes() const;

  using AttrSizeAttrFilterFn = bool (*)(const GLVertexFormatAttr&);
  auto doRecalculateSize(AttrSizeAttrFilterFn filter_fn, bool add_padding) const -> GLSize;

  // cached_sizes_ = std::nulopt;
  void invalidateCachedSizes();

  unsigned current_attrib_index_;
  AttributeArray attributes_;

  // A bitfield which represents vertex buffer indices
  //   which are referenced by the attributes
  //  - The LSB's state represents the allocation state of
  //    'bindingindex' 0, the MSB's - of the 31st
  unsigned vertex_buffer_bitfield_;

  // Has a '1' bit everywhere that
  //   - a 'AttrType::PerInstance' attribute has been bound
  //   - the corresponding bit in 'vertex_buffer_bitfield_'
  //     is also '1'
  unsigned instance_buffer_bitfield_;

  // A bitfield which stores the indices of bind points
  //   at which a vertex buffer is bound
  //  - The order of bits is the same as
  //    'vertex_buffer_bitfield_' i.e.
  //    the LSB is index 0, the MSB - index 31
  //  - Marked as 'mutable' as the array contains
  //    ephemeral data which 'const' methods
  //    sometimes need to purge
  mutable unsigned bound_vertex_buffer_bitfield_;

  // Stores info about bound vertex buffers required
  //   for the ARB_vertex_array_object code path(s).
  //  - 'Required' meaning - needed to make the two
  //    code paths (ARB_vertex_attrib_binding,
  //    ARB_vertex_array_object) as close to indisti-
  //    -nguishable from each other from the outside
  //    as possible
  //  - For reasoning behind marking this array as
  //    'mutable' - see comment above the member
  //    above 'bound_vertex_buffer_bitfield_'
  mutable BufferArray buffers_;

  // See comment above the padding() setter method
  GLSize padding_bytes_;

  // Computing the vertex/instance size is very expensive,
  //   so try to reduce the cost by caching it
  //  - Made 'mutable' so vertexByteSize() can
  //    fill it with the computed value
  struct CachedSizes {
    GLSize vertex;
    GLSize instance;
  };
  mutable std::optional<CachedSizes> cached_sizes_;

  int dbg_forced_va_create_path_;
};

class GLVertexArray : public GLObject {
public:
  struct VertexAttribBindingUnsupportedError : public std::runtime_error {
    VertexAttribBindingUnsupportedError() :
      std::runtime_error("ARB_vertex_attrib_binding support is required to do that!")
    { }
  };

  GLVertexArray(GLVertexArray&& other);
  virtual ~GLVertexArray();

  auto operator=(GLVertexArray&& other) -> GLVertexArray&;

  // TODO: this should be done in GLVertexFormat::createVertexArray(),
  //       not only for transparency of system detail to users of this
  //       class, but also because non-ARB_vertex_attrib_binding
  //       vertex arrays are completely broken right now :)
//  auto bindVertexBuffer(
//      unsigned index, const GLVertexBuffer& vertex_buffer,
//      GLSize stride, GLSize offset = 0
//    ) -> GLVertexArray&;

  // Bind this vertex array to the OpenGL context,
  //   so it will be used in subsequent draw calls
  //   until a bind() call on another vertex array
  //   is made
  auto bind() -> GLVertexArray&;

  auto unbind() -> GLVertexArray&;

protected:
  virtual auto doDestroy() -> GLObject& final;

private:
  friend GLVertexFormat;

  // GLVertexArray objects must be acquired through a GLVertexFormat
  //   to facillitate transparent use of 'ARB_vertex_attrib_binding'
  //   on machines which support this extension, or run OpenGL >=4.3
  //  - On older machines the old-style glVertexAttribPointer()
  //    family of functions will be used
  GLVertexArray();
};

// Private classes, enums, functions, variables etc.
namespace vertex_format_detail {
  enum CreateVertexArrayPath {
    Path_vertex_attrib_binding,
    Path_vertex_array_object,
  };
}

}
